﻿/***************************************************************************
 * Copyright (C) 2016 by Savoir-faire Linux                                *
 * Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
 * Author: Traczyk Andreas <andreas.traczyk@savoirfairelinux.com>          *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation; either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * This program is distributed in the hope that it will be useful,         *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 **************************************************************************/
#include "pch.h"
#include <string>
#include <direct.h>
#include "SmartPanel.xaml.h"
#include "qrencode.h"
#include <MemoryBuffer.h>   // IMemoryBufferByteAccess

using namespace Platform;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;
using namespace RingClientUWP::Views;
using namespace RingClientUWP::ViewModel;
using namespace Windows::Media::Capture;
using namespace Windows::Media::MediaProperties;
using namespace Windows::UI::Xaml;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Shapes;
using namespace Windows::UI::Xaml::Media;
using namespace Concurrency;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Foundation;
using namespace Concurrency;
using namespace Platform::Collections;

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Storage;
using namespace Windows::UI::Core;
using namespace Windows::System::Threading;

SmartPanel::SmartPanel()
{
    InitializeComponent();

    /* populate accounts listBox*/
    _accountsList_->ItemsSource = AccountListItemsViewModel::instance->itemsList;

    /* populate the smartlist */
    _smartList_->ItemsSource = SmartPanelItemsViewModel::instance->itemsList;


    /* populate the device list*/
///	_devicesIdList_ // not used so far

    /* connect delegates */
    Configuration::UserPreferences::instance->selectIndex += ref new SelectIndex([this](int index) {
        if (_accountsList_) {
            auto accountsListSize = dynamic_cast<Vector<AccountListItem^>^>(_accountsList_->ItemsSource)->Size;
            if (accountsListSize > index)
                _accountsList_->SelectedIndex = index;
            else {
                if (accountsListSize > 0)
                    _accountsList_->SelectedIndex = 0;
            }
        }
    });
    Configuration::UserPreferences::instance->loadProfileImage += ref new LoadProfileImage([this]() {
        StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
        String^ image_path = localfolder->Path + "\\.profile\\profile_image.png";
        auto uri = ref new Windows::Foundation::Uri(image_path);
        _selectedAccountAvatar_->ImageSource = ref new BitmapImage(uri);
    });
    ContactsViewModel::instance->contactDataModified += ref new ContactDataModified([this](Contact^ contact) {
    });
    AccountsViewModel::instance->updateScrollView += ref new UpdateScrollView([this]() {
        _accountsListScrollView_->UpdateLayout();
        _accountsListScrollView_->ScrollToVerticalOffset(_accountsListScrollView_->ScrollableHeight);
    });
    RingD::instance->incomingCall += ref new RingClientUWP::IncomingCall([&](
    String^ accountId, String^ callId, String^ from) {
        ///auto from = call->from;
        auto contact = ContactsViewModel::instance->findContactByRingId(from);

        if (contact == nullptr)
            contact = ContactsViewModel::instance->addNewContact(from, from); // contact checked inside addNewContact.

        if (contact == nullptr) {
            ERR_("contact not handled!");
            return;
        }

        RingD::instance->lookUpAddress(from);
        contact->_accountIdAssociated = accountId;

        auto item = SmartPanelItemsViewModel::instance->findItem(contact);
        item->_callId = callId;

        /* move the item of the top of the list */
        unsigned int index;
        if (_smartList_->Items->IndexOf(item, &index)) {
            SmartPanelItemsViewModel::instance->moveItemToTheTop(item);
            _smartList_->UpdateLayout();
            _smartList_->ScrollIntoView(item);
        }

    });
    RingD::instance->stateChange += ref new StateChange([this](String^ callId, CallStatus state, int code) {

        auto item = SmartPanelItemsViewModel::instance->findItem(callId);

        if (!item) {
            WNG_("item not found");
            return;
        }

        switch (state) {
        case CallStatus::NONE:
        case CallStatus::ENDED:
        {
            bool isInCall = false;
            for (auto item : SmartPanelItemsViewModel::instance->itemsList) { // WTF!!! item instead of it!!!! (XXX)
                if (item->_callId && item->_callStatus == CallStatus::IN_PROGRESS) {
                    isInCall = true;
                    RingD::instance->currentCallId = item->_callId;
                    break;
                }
            }
            if (!isInCall)
                _settingsMenuButton_->Visibility = VIS::Visible;
            break;
        }
        case CallStatus::IN_PROGRESS:
        {
            SmartPanelItemsViewModel::instance->_selectedItem = item;
            summonVideoPage();
            break;
        }
        case CallStatus::PEER_PAUSED:
        case CallStatus::PAUSED:
        {
            SmartPanelItemsViewModel::instance->_selectedItem = item;
            summonVideoPage();
            break;
        }
        default:
            break;
        }

    });
    RingD::instance->devicesListRefreshed += ref new RingClientUWP::DevicesListRefreshed(this, &RingClientUWP::Views::SmartPanel::OndevicesListRefreshed);

    ContactsViewModel::instance->contactAdded += ref new ContactAdded([this](Contact^ contact) {
        auto smartPanelItem = ref new SmartPanelItem();
        smartPanelItem->_contact = contact;
        SmartPanelItemsViewModel::instance->itemsList->Append(smartPanelItem);
    });

    RingD::instance->exportOnRingEnded += ref new RingClientUWP::ExportOnRingEnded(this, &RingClientUWP::Views::SmartPanel::OnexportOnRingEnded);
    RingD::instance->accountUpdated += ref new RingClientUWP::AccountUpdated(this, &RingClientUWP::Views::SmartPanel::OnaccountUpdated);
    RingD::instance->registeredNameFound += ref new RingClientUWP::RegisteredNameFound(this, &RingClientUWP::Views::SmartPanel::OnregisteredNameFound);

    RingD::instance->finishCaptureDeviceEnumeration += ref new RingClientUWP::FinishCaptureDeviceEnumeration([this]() {
        populateVideoDeviceSettingsComboBox();
    });
    RingD::instance->registrationStateErrorGeneric += ref new RingClientUWP::RegistrationStateErrorGeneric(this, &RingClientUWP::Views::SmartPanel::OnregistrationStateErrorGeneric);
    RingD::instance->registrationStateRegistered += ref new RingClientUWP::RegistrationStateRegistered(this, &RingClientUWP::Views::SmartPanel::OnregistrationStateRegistered);
    RingD::instance->callPlaced += ref new RingClientUWP::CallPlaced(this, &RingClientUWP::Views::SmartPanel::OncallPlaced);
    RingD::instance->incomingAccountMessage += ref new RingClientUWP::IncomingAccountMessage(this, &RingClientUWP::Views::SmartPanel::OnincomingAccountMessage);

    menuOpen = MenuOpen::CONTACTS_LIST;
}

void
RingClientUWP::Views::SmartPanel::updatePageContent()
{
    auto accountListItem = AccountListItemsViewModel::instance->_selectedItem;
    if (!accountListItem)
        return;

    auto name = accountListItem->_account->name_; // refacto remove name variable and use the link directly on the next line... like _upnpnState..._

    accountListItem->_isSelected = true;

    Configuration::UserPreferences::instance->PREF_ACCOUNT_INDEX = _accountsList_->SelectedIndex;
    Configuration::UserPreferences::instance->save();

    _selectedAccountName_->Text = name; // refacto : bind this in xaml directly
///    _devicesIdList_->ItemsSource = account->_devicesIdList;
    _deviceId_->Text = accountListItem->_account->_deviceId; /* this is the current device ...
    ... in the way to get all associated devices, we have to querry the daemon : */

    _devicesMenuButton_->Visibility = (accountListItem->_account->accountType_ == "RING")
                                      ? Windows::UI::Xaml::Visibility::Visible
                                      : Windows::UI::Xaml::Visibility::Collapsed;

    _shareMenuButton_->Visibility = (accountListItem->_account->accountType_ == "RING")
                                    ? Windows::UI::Xaml::Visibility::Visible
                                    : Windows::UI::Xaml::Visibility::Collapsed;

    _upnpState_->IsOn = accountListItem->_account->_upnpState;

    if (_upnpState_->IsOn) {
        _usernameTextBoxEdition_->IsEnabled = true;
        _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    } else {
        _usernameTextBoxEdition_->IsEnabled = false;
        _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

}

// refacto : rm me
void RingClientUWP::Views::SmartPanel::unselectContact()
{
    // _smartList_->SelectedItem = nullptr;
}

void RingClientUWP::Views::SmartPanel::_smartGridButton__Clicked(Object^ sender, RoutedEventArgs^ e)
{
    _accountsMenuButton__Unchecked(nullptr,nullptr);
    _accountsMenuButton_->IsChecked = false;
    _shareMenuButton__Unchecked(nullptr,nullptr);
    _shareMenuButton_->IsChecked = false;
    _devicesMenuButton__Unchecked(nullptr,nullptr);
    _devicesMenuButton_->IsChecked = false;
    _settingsMenu_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _settingsMenuButton_->IsChecked = false;

    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void RingClientUWP::Views::SmartPanel::_accountsMenuButton__Checked(Object^ sender, RoutedEventArgs^ e)
{
    menuOpen = MenuOpen::ACCOUNTS_LIST;
    _settingsMenu_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _settingsMenuButton_->IsChecked = false;
    _shareMenuButton_->IsChecked = false;
    _devicesMenuButton_->IsChecked = false;
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _devicesMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void RingClientUWP::Views::SmartPanel::_accountsMenuButton__Unchecked(Object^ sender, RoutedEventArgs^ e)
{
    menuOpen = MenuOpen::CONTACTS_LIST;
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountEditionGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountAddMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void RingClientUWP::Views::SmartPanel::_settingsMenu__Checked(Object^ sender, RoutedEventArgs^ e)
{
    menuOpen - MenuOpen::SETTINGS;
    _accountsMenuButton__Unchecked(nullptr,nullptr);
    _accountsMenuButton_->IsChecked = false;
    _shareMenuButton__Unchecked(nullptr,nullptr);
    _shareMenuButton_->IsChecked = false;
    _devicesMenuButton__Unchecked(nullptr,nullptr);
    _devicesMenuButton_->IsChecked = false;
    _settingsMenu_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    auto vcm = Video::VideoManager::instance->captureManager();
    if (vcm->deviceList->Size > 0) {
        if (!vcm->isInitialized)
            vcm->InitializeCameraAsync(true);
        else
            vcm->StartPreviewAsync(true);
    }
    summonPreviewPage();
}

void RingClientUWP::Views::SmartPanel::_settingsMenu__Unchecked(Object^ sender, RoutedEventArgs^ e)
{
    menuOpen = MenuOpen::CONTACTS_LIST;
    _settingsMenu_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    auto vcm = Video::VideoManager::instance->captureManager();
    if (vcm->deviceList->Size > 0) {
        vcm->StopPreviewAsync()
        .then([](task<void> stopPreviewTask)
        {
            try {
                stopPreviewTask.get();
                Video::VideoManager::instance->captureManager()->isSettingsPreviewing = false;
            }
            catch (Exception^ e) {
                WriteException(e);
            }
        });
    }
    hidePreviewPage();
}

void RingClientUWP::Views::SmartPanel::setMode(RingClientUWP::Views::SmartPanel::Mode mode)
{
    if (mode == RingClientUWP::Views::SmartPanel::Mode::Normal) {
        _rowRingTxtBx_->Height = 40;
        _selectedAccountAvatarContainer_->Height = 80;
        _shaderPhotoboothIcon_->Height = 80;
        _selectedAccountAvatarColumn_->Width = 90;
        _selectedAccountRow_->Height = 90;
    }
    else {
        _rowRingTxtBx_->Height = 0;
        _selectedAccountAvatarContainer_->Height = 50;
        _shaderPhotoboothIcon_->Height = 50;
        _selectedAccountAvatarColumn_->Width = 60;
        _selectedAccountRow_->Height = 60;
    }

    _selectedAccountAvatarContainer_->Width = _selectedAccountAvatarContainer_->Height;
    _shaderPhotoboothIcon_->Width = _shaderPhotoboothIcon_->Height;
    _settingsMenuButton_->IsChecked = false;
    _accountsMenuButton_->IsChecked = false;
    _shareMenuButton_->IsChecked = false;
    _devicesMenuButton_->IsChecked = false;
}

void RingClientUWP::Views::SmartPanel::_shareMenuButton__Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    menuOpen = MenuOpen::SHARE;
    _settingsMenu_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _settingsMenuButton_->IsChecked = false;
    _shareMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountEditionGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _devicesMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountsMenuButton_->IsChecked = false;
    _devicesMenuButton_->IsChecked = false;

    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    generateQRcode();
}

void RingClientUWP::Views::SmartPanel::_shareMenuButton__Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    menuOpen = MenuOpen::CONTACTS_LIST;
    _shareMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountEditionGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void RingClientUWP::Views::SmartPanel::_addAccountBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _createAccountYes_->IsEnabled = false;

    _accountTypeComboBox_->SelectedIndex = 0;
    _upnpStateAccountCreation_->IsOn = true;
    _RegisterStateEdition_->IsOn = true;
    _accountAliasTextBox_->Text = "";
    _usernameTextBox_->Text = "";

    checkStateAddAccountMenu();
}

void RingClientUWP::Views::SmartPanel::_createAccountYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    switch (_accountTypeComboBox_->SelectedIndex) {
    case 0: /* RING account */
    {
        RingD::instance->createRINGAccount(_accountAliasTextBox_->Text
                                           , _ringPasswordBoxAccountCreation_->Password
                                           , _upnpStateAccountCreation_->IsOn
                                           , (_RegisterState_->IsOn) ? _usernameTextBox_->Text : "");

        _ringPasswordBoxAccountCreation_->Password = "";
        _ringPasswordBoxAccountCreationCheck_->Password = "";
        _usernameTextBox_->Text = "";
        break;
    }
    break;
    case 1: /* SIP account */
    {
        RingD::instance->createSIPAccount(_accountAliasTextBox_->Text, _sipPasswordBoxAccountCreation_->Password, _sipHostnameTextBox_->Text, _sipUsernameTextBox_->Text);
        _sipPasswordBoxAccountCreation_->Password = "";
        _sipUsernameTextBox_->Text = "";
        _sipHostnameTextBox_->Text = "";
        break;
    }
    default:
        break;
    }

    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountsMenuButton__Checked(nullptr, nullptr);
}


void RingClientUWP::Views::SmartPanel::_createAccountNo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _accountsMenuButton_->IsChecked = false;
    _accountsMenuButton__Unchecked(nullptr,nullptr);
}

/* using the usual selection behaviour doesn't allow us to deselect by simple click. The selection is managed
   by Grid_PointerReleased */
void
SmartPanel::_smartList__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    auto listbox = dynamic_cast<ListBox^>(sender); // same as _smartList_
    listbox->SelectedItem = nullptr;
    return;
}

void
SmartPanel::_accountList__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    auto listbox = safe_cast<ListBox^>(sender);
    // disable deselction from listbox
    if (listbox->SelectedItem == nullptr)
    {
        if (e->RemovedItems->Size > 0)
        {
            Object^ itemToReselect = e->RemovedItems->GetAt(0);
            for each (auto item in listbox->Items) {
                if (item == itemToReselect) {
                    listbox->SelectedItem = itemToReselect;
                    continue;
                }
            }
        }
    }
    auto account = safe_cast<AccountListItem^>(listbox->SelectedItem);
    AccountListItemsViewModel::instance->_selectedItem = account;

    updatePageContent();
}

// (XXX) use only KeyUp
void RingClientUWP::Views::SmartPanel::_ringTxtBx__KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    /* add contact, test purpose but will be reused later in some way */
    //if (e->Key == Windows::System::VirtualKey::Enter && !_ringTxtBx_->Text->IsEmpty()) {
    //    for (auto it : SmartPanelItemsViewModel::instance->itemsList) {
    //        if (it->_contact->name_ == _ringTxtBx_->Text) {
    //            _smartList_->SelectedItem = it;
    //            _ringTxtBx_->Text = "";
    //            return;
    //        }
    //    }

    //    /* if the string has 40 chars, we simply consider it as a ring id. It has to be improved */
    //    if (_ringTxtBx_->Text->Length() == 40) {
    //        ContactsViewModel::instance->addNewContact(_ringTxtBx_->Text, _ringTxtBx_->Text);
    //        _ringTxtBx_->Text = "";
    //    }


    //    RingD::instance->lookUpName(_ringTxtBx_->Text);
    //}
}

void RingClientUWP::Views::SmartPanel::_ringTxtBx__Click(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    RingD::instance->lookUpName(_ringTxtBx_->Text);
}

void
RingClientUWP::Views::SmartPanel::_rejectIncomingCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    if (button) {
        auto item = dynamic_cast<SmartPanelItem^>(button->DataContext);
        if (item) {
            auto callId = item->_callId;
            RingD::instance->refuseIncommingCall(callId);
        }
    }
}

void
RingClientUWP::Views::SmartPanel::_acceptIncomingCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    if (button) {
        auto item = dynamic_cast<SmartPanelItem^>(button->DataContext);
        if (item) {
            auto callId = item->_callId;

            for (auto it : SmartPanelItemsViewModel::instance->itemsList)
                if (it->_callStatus != CallStatus::IN_PROGRESS)
                    RingD::instance->pauseCall(Utils::toString(it->_callId));


            RingD::instance->acceptIncommingCall(callId);
        }
    }
}

void
SmartPanel::_callContact__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    if (button) {
        /* force to hide the button, avoid attempting to call several times... */
        button->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

        _settingsMenuButton_->Visibility = VIS::Collapsed;

        auto item = dynamic_cast<SmartPanelItem^>(button->DataContext);
        if (item) {
            auto contact = item->_contact;
            if (contact)
            {
                for (auto it : SmartPanelItemsViewModel::instance->itemsList)
                    if (it->_callStatus == CallStatus::IN_PROGRESS)
                        RingD::instance->pauseCall(Utils::toString(it->_callId));

                if (item->_callStatus == CallStatus::ENDED || item->_callStatus == CallStatus::NONE) {
                    item->_callStatus = CallStatus::OUTGOING_REQUESTED;
                    RingD::instance->placeCall(contact);
                    item->_contact->_lastTime = "looking for " + item->_contact->_name + ".";
                }

                /* move the item of the top of the list */
                unsigned int index;
                if (_smartList_->Items->IndexOf(item, &index)) {
                    SmartPanelItemsViewModel::instance->moveItemToTheTop(item);
                    _smartList_->UpdateLayout();
                    _smartList_->ScrollIntoView(item);
                }
            }
        }
    }
}

void RingClientUWP::Views::SmartPanel::_cancelCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    if (button) {
        auto item = dynamic_cast<SmartPanelItem^>(button->DataContext);
        if (item) {
            RingD::instance->cancelOutGoingCall2(item->_callId);
            item->_callStatus = CallStatus::TERMINATING;
            return;
        }
    }
}

void RingClientUWP::Views::SmartPanel::Grid_PointerEntered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto grid = dynamic_cast<Grid^>(sender);
    auto item = dynamic_cast<SmartPanelItem^>(grid->DataContext);

    for (auto it : SmartPanelItemsViewModel::instance->itemsList) {
        it->_isHovered = false;
    }

    item->_isHovered = true;
}


void RingClientUWP::Views::SmartPanel::Grid_PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto grid = dynamic_cast<Grid^>(sender);
    auto item = dynamic_cast<SmartPanelItem^>(grid->DataContext);

    // to avoid visual bug, do it on the whole list
    for each (auto it in SmartPanelItemsViewModel::instance->itemsList) {
        it->_isHovered = false;
    }
}

// refacto : rm
void RingClientUWP::Views::SmartPanel::_contactItem__PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    /*auto listBoxItem = dynamic_cast<ListBoxItem^>(sender);
    auto smartPanelItem = dynamic_cast<SmartPanelItem^>(listBoxItem->DataContext);

    if (_smartList_->SelectedItem != smartPanelItem)
        _smartList_->SelectedItem = smartPanelItem;
    else
        _smartList_->SelectedItem = nullptr;
    */
}

void RingClientUWP::Views::SmartPanel::generateQRcode()
{
    auto ringId = AccountListItemsViewModel::instance->_selectedItem->_account->ringID_;
    auto ringId2 = Utils::toString(ringId);

    _ringId_->Text = ringId;

    auto qrcode = QRcode_encodeString(ringId2.c_str(),
                                      0, //Let the version be decided by libqrencode
                                      QR_ECLEVEL_L, // Lowest level of error correction
                                      QR_MODE_8, // 8-bit data mode
                                      1);

    if (!qrcode) {
        WNG_("Failed to generate QR code: ");
        return;
    }

    const int STRETCH_FACTOR = 4;
    const int widthQrCode = qrcode->width;
    const int widthBitmap = STRETCH_FACTOR * widthQrCode;

    unsigned char* qrdata = qrcode->data;

    auto frame = ref new SoftwareBitmap(BitmapPixelFormat::Bgra8, widthBitmap, widthBitmap, BitmapAlphaMode::Premultiplied);

    const int BYTES_PER_PIXEL = 4;


    BitmapBuffer^ buffer = frame->LockBuffer(BitmapBufferAccessMode::ReadWrite);
    IMemoryBufferReference^ reference = buffer->CreateReference();

    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> byteAccess;
    if (SUCCEEDED(reinterpret_cast<IUnknown*>(reference)->QueryInterface(IID_PPV_ARGS(&byteAccess))))
    {
        byte* data;
        unsigned capacity;
        byteAccess->GetBuffer(&data, &capacity);

        auto desc = buffer->GetPlaneDescription(0);

        unsigned char* row, * p;
        p = qrcode->data;

        for (int u = 0 ; u < widthBitmap ; u++) {
            for (int v = 0; v < widthBitmap; v++) {
                int x = static_cast<int>((float)u / (float)widthBitmap * (float)widthQrCode);
                int y = static_cast<int>((float)v / (float)widthBitmap * (float)widthQrCode);

                auto currPixelRow = desc.StartIndex + desc.Stride * u + BYTES_PER_PIXEL * v;
                row = (p + (y * widthQrCode));

                if (*(row + x) & 0x1) {
                    data[currPixelRow + 3] = 255;
                }
            }
        }

    }
    delete reference;
    delete buffer;

    auto sbSource = ref new Media::Imaging::SoftwareBitmapSource();

    sbSource->SetBitmapAsync(frame);

    _selectedAccountQrCode_->Source = sbSource;
}

void RingClientUWP::Views::SmartPanel::checkStateAddAccountMenu()
{
    bool isRingAccountType = (_accountTypeComboBox_->SelectedIndex == 0) ? true : false;

    bool isAccountAlias = (_accountAliasTextBox_->Text->IsEmpty()) ? false : true;

    if (isAccountAlias) {
        _accountAliasValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _accountAliasInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    } else {
        _accountAliasValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _accountAliasInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    if (isRingAccountType) {
        bool isPublic = _RegisterState_->IsOn;

        bool isUsernameValid = (_usernameValid_->Visibility == Windows::UI::Xaml::Visibility::Visible
                                && !_usernameTextBox_->Text->IsEmpty()) ? true : false;

        bool isPasswordValid = (_ringPasswordBoxAccountCreation_->Password->IsEmpty()) ? false : true;

        bool isRingPasswordCheck = (_ringPasswordBoxAccountCreation_->Password
                                    == _ringPasswordBoxAccountCreationCheck_->Password
                                    && isPasswordValid)
                                   ? true : false;

        if (isPasswordValid) {
            _passwordValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
            _passwordInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        }
        else {
            _passwordValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            _passwordInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        }

        if (isRingPasswordCheck) {
            _passwordCheckValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
            _passwordCheckInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        }
        else {
            _passwordCheckValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            _passwordCheckInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        }

        if (isUsernameValid) {
            _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
            _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        } else {
            _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        }

        if (isPublic)
            if (isUsernameValid && isAccountAlias && isRingPasswordCheck && isPasswordValid)
                _createAccountYes_->IsEnabled = true;
            else
                _createAccountYes_->IsEnabled = false;
        else if (isAccountAlias && isRingPasswordCheck && isPasswordValid)
            _createAccountYes_->IsEnabled = true;
        else
            _createAccountYes_->IsEnabled = false;

    } else {
        if (isAccountAlias)
            _createAccountYes_->IsEnabled = true;
        else
            _createAccountYes_->IsEnabled = false;
    }

}

void RingClientUWP::Views::SmartPanel::checkStateEditionMenu()
{
    if (AccountListItemsViewModel::instance->_selectedItem == nullptr)
        return;
    bool isRingAccountType = (AccountListItemsViewModel::instance->_selectedItem->_account->accountType_ == "RING")
                             ? true : false;

    bool isAccountAlias = (_accountAliasTextBoxEdition_->Text->IsEmpty()) ? false : true;

    bool isAlreadyRegistered = (_RegisterStateEdition_->IsEnabled) ? false : true;

    if (isAccountAlias) {
        _accountAliasValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _accountAliasInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
    else {
        _accountAliasValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _accountAliasInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    }

    if (isRingAccountType) {
        bool isPublic = _RegisterStateEdition_->IsOn;

        bool isUsernameValid = (_usernameValidEdition_->Visibility == Windows::UI::Xaml::Visibility::Visible)
                               ? true : false;

        bool isPasswordValid = (_ringPasswordBoxAccountCreation_->Password->IsEmpty()) ? false : true;

        bool isRingPasswordCheck = (_ringPasswordBoxAccountCreation_->Password
                                    == _ringPasswordBoxAccountCreationCheck_->Password
                                    && isPasswordValid)
                                   ? true : false;
        if (isPublic)
            if (isUsernameValid && isAccountAlias || isAlreadyRegistered) {
                _acceptAccountModification_->IsEnabled = true;
                //_registerOnBlockchainEdition_->IsEnabled = true;
            } else {
                _acceptAccountModification_->IsEnabled = false;
                //_registerOnBlockchainEdition_->IsEnabled = false;
            }
        else if (isAccountAlias)
            _acceptAccountModification_->IsEnabled = true;
        else
            _acceptAccountModification_->IsEnabled = false;
    }
    else {
        if (isAccountAlias)
            _acceptAccountModification_->IsEnabled = true;
        else
            _acceptAccountModification_->IsEnabled = false;
    }
}

void RingClientUWP::Views::SmartPanel::ringTxtBxPlaceHolderDelay(String^ placeHolderText, int delayInMilliSeconds)
{
    _ringTxtBx_->PlaceholderText = placeHolderText;
    TimeSpan delay;
    delay.Duration = 10000 * delayInMilliSeconds;
    ThreadPoolTimer^ delayTimer = ThreadPoolTimer::CreateTimer(
                                      ref new TimerElapsedHandler([this](ThreadPoolTimer^ source)
    {
        Dispatcher->RunAsync(CoreDispatcherPriority::High,
                             ref new DispatchedHandler([this]()
        {
            _ringTxtBx_->PlaceholderText = "";
        }));
    }), delay);
}

void RingClientUWP::Views::SmartPanel::showLinkThisDeviceStep1()
{
    _step1Menu_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _step2Menu_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    _nextstep_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _addAccountYes_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _addAccountNo_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

Object ^ RingClientUWP::Views::IncomingVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto state = static_cast<CallStatus>(value);

    if (state == CallStatus::INCOMING_RINGING)
        return  Windows::UI::Xaml::Visibility::Visible;
    else
        return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object ^ RingClientUWP::Views::IncomingVisibility::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::IncomingVisibility::IncomingVisibility()
{}


Object ^ RingClientUWP::Views::OutGoingVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto state = static_cast<CallStatus>(value);

    if (state == CallStatus::SEARCHING
            || state == CallStatus::OUTGOING_RINGING
            || state == CallStatus::OUTGOING_REQUESTED)
        return  Windows::UI::Xaml::Visibility::Visible;
    else
        return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object ^ RingClientUWP::Views::OutGoingVisibility::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::OutGoingVisibility::OutGoingVisibility()
{}

Object ^ RingClientUWP::Views::HasAnActiveCall::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto state = static_cast<CallStatus>(value);

    if (state == CallStatus::NONE || state == CallStatus::ENDED)
        return Windows::UI::Xaml::Visibility::Collapsed;
    else
        return Windows::UI::Xaml::Visibility::Visible;
}

Object ^ RingClientUWP::Views::HasAnActiveCall::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::HasAnActiveCall::HasAnActiveCall()
{}

Object ^ RingClientUWP::Views::NewMessageBubleNotification::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto unreadMessages = static_cast<uint32>(value);

    if (unreadMessages > 0)
        return Windows::UI::Xaml::Visibility::Visible;

    return Windows::UI::Xaml::Visibility::Collapsed;
}

Object ^ RingClientUWP::Views::NewMessageBubleNotification::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::NewMessageBubleNotification::NewMessageBubleNotification()
{}

void RingClientUWP::Views::SmartPanel::_devicesMenuButton__Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    menuOpen = MenuOpen::CONTACTS_LIST;
    _devicesMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _waitingForPin_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _passwordForPinGenerator_->Password = "";
    // refacto : do something better...
    _waitingAndResult_->Text = "Exporting account on the Ring...";

    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}


void RingClientUWP::Views::SmartPanel::_devicesMenuButton__Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    menuOpen = MenuOpen::DEVICE;
    _settingsMenu_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _settingsMenuButton_->IsChecked = false;

    _waitingDevicesList_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _devicesIdList_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    auto account = AccountListItemsViewModel::instance->_selectedItem->_account;

    _deviceId_->Text = account->_deviceId;
    if (_deviceId_->Text->IsEmpty()) {
        _deviceId_->Text = "No device id found!";
        ERR_("device Id not found for account " + Utils::toString(account->_deviceId));
    }

    RingD::instance->askToRefreshKnownDevices(account->accountID_);

    _shareMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _devicesMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountsMenuButton_->IsChecked = false;
    _shareMenuButton_->IsChecked = false;

    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}


void RingClientUWP::Views::SmartPanel::_addDevice__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _devicesMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _closePin_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}


void RingClientUWP::Views::SmartPanel::OndevicesListRefreshed(Platform::Collections::Vector<Platform::String ^, std::equal_to<Platform::String ^>, true> ^devicesList)
{
    _waitingDevicesList_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    AccountListItemsViewModel::instance->_selectedItem->_account->_devicesIdList = devicesList;
    _devicesIdList_->ItemsSource = devicesList;
    _devicesIdList_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}


void RingClientUWP::Views::SmartPanel::_pinGeneratorYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _waitingForPin_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    auto accountId = AccountListItemsViewModel::instance->_selectedItem->_account->accountID_;
    auto password = _passwordForPinGenerator_->Password;
    _passwordForPinGenerator_->Password = "";

    /* hide the button while we are waiting... */
    _closePin_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    RingD::instance->askToExportOnRing(accountId, password);
}


void RingClientUWP::Views::SmartPanel::_pinGeneratorNo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _devicesMenuButton_->IsChecked = false;
    _passwordForPinGenerator_->Password = "";
}


void RingClientUWP::Views::SmartPanel::OnexportOnRingEnded(Platform::String ^accountId, Platform::String ^pin)
{
    _waitingAndResult_->Text = pin;
    _closePin_->Visibility = Windows::UI::Xaml::Visibility::Visible;

}


void RingClientUWP::Views::SmartPanel::_closePin__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _waitingForPin_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _devicesMenuButton_->IsChecked = false;

    // refacto : do something better...
    _waitingAndResult_->Text = "Exporting account on the Ring...";
}


void RingClientUWP::Views::SmartPanel::_shareMenuDone__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _shareMenuButton_->IsChecked = false;

    _shareMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

Object ^ RingClientUWP::Views::AccountTypeToSourceImage::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto accountType = dynamic_cast<String^>(value);
    Uri^ uri = (accountType == "RING")
               ? ref new Uri("ms-appx:///Assets/AccountTypeRING.png")
               : ref new Uri("ms-appx:///Assets/AccountTypeSIP.png");

    return ref new BitmapImage(uri);
}

Object ^ RingClientUWP::Views::AccountTypeToSourceImage::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::AccountTypeToSourceImage::AccountTypeToSourceImage()
{}

Object ^ RingClientUWP::Views::AccountSelectedToVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    //auto accountId = static_cast<bool(value);

    if (/*AccountListItemsViewModel::instance->_selectedItem->_account->_isSelected ==*/ (bool)value == true)
        return Windows::UI::Xaml::Visibility::Visible;

    return Windows::UI::Xaml::Visibility::Collapsed;
}

Object ^ RingClientUWP::Views::AccountSelectedToVisibility::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::AccountSelectedToVisibility::AccountSelectedToVisibility()
{}


void RingClientUWP::Views::SmartPanel::_editAccountMenuButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto account = AccountListItemsViewModel::instance->_selectedItem->_account;

    auto volatileAccountDetails = RingD::instance->getVolatileAccountDetails(account);

    _accountAliasTextBoxEdition_->Text = account->name_;
    _accountEditionGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _deleteAccountEdition_->IsOn = false;
    _deleteAccountEdition_->IsEnabled = (AccountListItemsViewModel::instance->itemsList->Size > 1)? true : false;
    _createAccountYes_->IsEnabled = false;

    _whatWilHappendeleteRingAccountEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _whatWilHappendeleteSipAccountEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    _sipAccountStackEdition_->Visibility = (account->accountType_ == "SIP")
                                           ? Windows::UI::Xaml::Visibility::Visible
                                           :Windows::UI::Xaml::Visibility::Collapsed;
    _ringStackEdition_->Visibility = (account->accountType_ == "RING")
                                     ? Windows::UI::Xaml::Visibility::Visible
                                     : Windows::UI::Xaml::Visibility::Collapsed;
    _sipHostnameEdition_->Text = account->_sipHostname;
    _sipUsernameEditionTextBox_->Text = account->_sipUsername;
    _sipPasswordEdition_->Password = account->_sipPassword;

    auto registeredName = Utils::toPlatformString(RingD::instance->registeredName(account));

    if (registeredName) {
        _RegisterStateEdition_->IsOn = true; // keep this before _usernameTextBoxEdition_

        _usernameTextBoxEdition_->IsEnabled = false;
        _usernameTextBoxEdition_->Text = registeredName;
        _RegisterStateEdition_->IsEnabled = false;

        //_registerOnBlockchainEdition_->IsEnabled = false;
        _whatWilHappenEdition_->Text = "You are already registered.";
        _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    } else {
        _RegisterStateEdition_->IsOn = false;

        _usernameTextBoxEdition_->IsEnabled = false;
        _usernameTextBoxEdition_->Text = "";
        _RegisterStateEdition_->IsEnabled = true;

        //_registerOnBlockchainEdition_->IsEnabled = false;
        _whatWilHappenEdition_->Text = "You are not yet registered.";
        _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    checkStateEditionMenu();
}


void RingClientUWP::Views::SmartPanel::_acceptAccountModification__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto account = AccountListItemsViewModel::instance->_selectedItem->_account;
    auto accountId = account->accountID_;


    // mettre ca en visibility du bouton delete
    auto accountsListSize = dynamic_cast<Vector<AccountListItem^>^>(_accountsList_->ItemsSource)->Size;

    /* if the delete button is toggled, just delete the account */
    if (_deleteAccountEdition_->IsOn == true && accountsListSize > 1) {
        AccountListItem^ item;
        for each (item in AccountListItemsViewModel::instance->itemsList)
            if (item->_account->accountID_ == accountId)
                break;

        if (item)
            AccountListItemsViewModel::instance->removeItem(item);

        RingD::instance->deleteAccount(accountId);

    } else { /* otherwise edit the account */

        account->name_ = _accountAliasTextBoxEdition_->Text;

        if (account->accountType_ == "RING") {
            account->_upnpState = _upnpState_->IsOn;
        }
        else {
            account->_sipHostname = _sipHostnameEdition_->Text;
            account->_sipUsername = _sipUsernameEditionTextBox_->Text;
            account->_sipPassword = _sipPasswordEdition_->Password;
        }

        RingD::instance->updateAccount(accountId);
    }

    _accountEditionGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountsMenuButton_->IsChecked = false;

    if (_usernameValidEdition_->Visibility == Windows::UI::Xaml::Visibility::Visible && _usernameTextBoxEdition_->Text->Length() > 2)
        RingD::instance->registerName_new(Utils::toString(account->accountID_), "", Utils::toString(_usernameTextBoxEdition_->Text));
}


void RingClientUWP::Views::SmartPanel::_cancelAccountModification__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _accountEditionGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}


void RingClientUWP::Views::SmartPanel::OnaccountUpdated(RingClientUWP::Account ^account)
{
    updatePageContent();
}

void RingClientUWP::Views::SmartPanel::_passwordBoxAccountCreationCheck__PasswordChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    checkStateAddAccountMenu();
}


void RingClientUWP::Views::SmartPanel::_accountTypeComboBox__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    auto accountTypeComboBox = dynamic_cast<ComboBox^>(sender);

    /* avoid exception at start */
    if (_ringAccountCreationStack_ == nullptr && _sipAccountCreationStack_ == nullptr)
        return;

    /* empty everything, avoid to keep credentials in memory... */
    _ringPasswordBoxAccountCreation_->Password = "";
    _ringPasswordBoxAccountCreationCheck_->Password = "";
    _sipPasswordBoxAccountCreation_->Password = "";
    _sipUsernameTextBox_->Text = "";
    _sipHostnameTextBox_->Text = "";

    if (accountTypeComboBox->SelectedIndex == 0 /* RING type is selected */) {
        _ringAccountCreationStack_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _sipAccountCreationStack_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _createAccountYes_->IsEnabled = false;
        _upnpStateAccountCreation_->IsOn = true;
    } else { /* SIP type is selected */
        _ringAccountCreationStack_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _sipAccountCreationStack_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _createAccountYes_->IsEnabled = (_accountAliasTextBox_->Text->IsEmpty()) ? false : true;
    }
}




void RingClientUWP::Views::SmartPanel::_accountAliasTextBox__TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
    checkStateAddAccountMenu();
}

void RingClientUWP::Views::SmartPanel::_accountAliasTextBoxEdition__TextChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::TextChangedEventArgs^ e)
{
    checkStateEditionMenu();
}

Object ^ RingClientUWP::Views::CollapseEmptyString::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto stringValue = dynamic_cast<String^>(value);

    return (stringValue->IsEmpty())
           ? Windows::UI::Xaml::Visibility::Collapsed
           : Windows::UI::Xaml::Visibility::Visible;
}

Object ^ RingClientUWP::Views::CollapseEmptyString::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::CollapseEmptyString::CollapseEmptyString()
{}


void RingClientUWP::Views::SmartPanel::_selectedAccountAvatarContainer__PointerEntered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    _photoboothIcon_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _shaderPhotoboothIcon_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}


void
RingClientUWP::Views::SmartPanel::_selectedAccountAvatarContainer__PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    CameraCaptureUI^ cameraCaptureUI = ref new CameraCaptureUI();
    cameraCaptureUI->PhotoSettings->Format = CameraCaptureUIPhotoFormat::Jpeg;
    cameraCaptureUI->PhotoSettings->CroppedSizeInPixels = Size(80, 80);

    create_task(cameraCaptureUI->CaptureFileAsync(CameraCaptureUIMode::Photo))
    .then([this](StorageFile^ photoFile)
    {
        if (photoFile != nullptr) {
            auto brush = ref new ImageBrush();

            auto circle = ref new Ellipse();
            circle->Height = 80;
            circle->Width = 80;
            auto path = photoFile->Path;
            auto uri = ref new Windows::Foundation::Uri(path);
            auto bitmapImage = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage();
            bitmapImage->UriSource = uri;

            std::string fileBuffer = Utils::getStringFromFile(Utils::toString(photoFile->Path));
            std::string profilePath = RingD::instance->getLocalFolder() + ".profile";
            _mkdir(profilePath.c_str());
            std::ofstream file((profilePath + "\\profile_image.png"),
                               std::ios::out | std::ios::trunc | std::ios::binary);
            if (file.is_open()) {
                file << fileBuffer;
                file.close();
            }

            Configuration::UserPreferences::instance->PREF_PROFILE_HASPHOTO = true;
            Configuration::UserPreferences::instance->save();

            brush->ImageSource = bitmapImage;
            circle->Fill = brush;
            _selectedAccountAvatar_->ImageSource = bitmapImage;
        }
    });
}


void RingClientUWP::Views::SmartPanel::_selectedAccountAvatarContainer__PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    _photoboothIcon_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _shaderPhotoboothIcon_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}


void RingClientUWP::Views::SmartPanel::_smartList__PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{

}


void RingClientUWP::Views::SmartPanel::Grid_PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto grid = dynamic_cast<Grid^>(sender);
    auto item = dynamic_cast<SmartPanelItem^>(grid->DataContext);

    for (auto it : SmartPanelItemsViewModel::instance->itemsList)
        it->_isHovered = false;

    item->_isHovered = true;
}

// NAME SERVICE

void RingClientUWP::Views::SmartPanel::_registerOnBlockchainEdition__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto account = AccountListItemsViewModel::instance->_selectedItem->_account;

    //RingD::instance->registerName(account->accountID_, "", _usernameTextBoxEdition_->Text);
    RingD::instance->registerName_new(Utils::toString(account->accountID_), "", Utils::toString(_usernameTextBoxEdition_->Text));
}


void RingClientUWP::Views::SmartPanel::_usernameTextBoxEdition__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    RingD::instance->lookUpName(_usernameTextBoxEdition_->Text);

    _usernameValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    checkStateEditionMenu();
}

void RingClientUWP::Views::SmartPanel::OnregisteredNameFound(RingClientUWP::LookupStatus status, const std::string& address, const std::string& name)
{
    if (menuOpen == MenuOpen::ACCOUNTS_LIST) { // if true, we did the lookup for a new account
        /* note : this code do both check for edit and creation menu. It doesn't affect the use and it's easier to
           implement. */
        auto currentNameEdition = Utils::toString(_usernameTextBoxEdition_->Text);
        if (currentNameEdition == name) {
            switch (status)
            {
            case LookupStatus::SUCCESS:
                _usernameValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Visible;
                break;
            case LookupStatus::INVALID_NAME:
                _usernameValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Visible;
                break;
            case LookupStatus::NOT_FOUND:
                _usernameValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Visible;
                _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                break;
            case LookupStatus::ERRORR:
                _usernameValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Visible;
                break;
            }
            checkStateEditionMenu();
            return;
        }

        auto currentNameCreation = Utils::toString(_usernameTextBox_->Text);
        if (currentNameCreation == name) {
            switch (status)
            {
            case LookupStatus::SUCCESS:
                _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
                break;
            case LookupStatus::INVALID_NAME:
                _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
                break;
            case LookupStatus::NOT_FOUND:
                _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
                _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                break;
            case LookupStatus::ERRORR:
                _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
                _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
                break;
            }
            checkStateAddAccountMenu();
            return;
        }

    }
    else { // if false, we are looking for a registered user (contact)
        auto contact = ContactsViewModel::instance->findContactByName(Utils::toPlatformString(name));

        if (contact == nullptr)
            return;

        switch (status) {
        case LookupStatus::SUCCESS:
        {
            if (contact->_contactStatus == ContactStatus::WAITING_FOR_ACTIVATION) {
                contact->_contactStatus = ContactStatus::READY;
                contact->ringID_ = Utils::toPlatformString(address);
                ringTxtBxPlaceHolderDelay("username found and added.", 5000);
                ContactsViewModel::instance->saveContactsToFile();
            }
            else {
                /* in that case we delete a possible suroggate */
                for each (Contact^ co in ContactsViewModel::instance->contactsList) {
                    if (co->_contactStatus == ContactStatus::WAITING_FOR_ACTIVATION
                            && co->_name == Utils::toPlatformString(name)) {
                        auto item = SmartPanelItemsViewModel::instance->findItem(co);
                        ContactsViewModel::instance->deleteContact(co);
                        SmartPanelItemsViewModel::instance->removeItem(item);
                    }

                }

            }

            /* open the text message page */
            auto item = SmartPanelItemsViewModel::instance->findItem(contact);

            //if (item->_callStatus != CallStatus::INCOMING_RINGING) {// refacto : do something better
            //    SmartPanelItemsViewModel::instance->_selectedItem = item;
            //    summonMessageTextPage();
            //}
        }
        break;
        case LookupStatus::INVALID_NAME:
            if (name.length() == 40) {

                /* first we check if some contact is registred with this ring id */
                auto contactAlreadyRecorded = ContactsViewModel::instance->findContactByRingId(Utils::toPlatformString(name));
                if (contactAlreadyRecorded) {
                    ringTxtBxPlaceHolderDelay("you already have a contact with this ring id.", 5000);
                    /* delete the contact added recently */
                    auto item = SmartPanelItemsViewModel::instance->findItem(contact);
                    ContactsViewModel::instance->deleteContact(contact);
                    SmartPanelItemsViewModel::instance->removeItem(item);

                    /* open the message text with the contact already recorder*/
                    item = SmartPanelItemsViewModel::instance->findItem(contactAlreadyRecorded);

                    //if (item->_callStatus != CallStatus::INCOMING_RINGING) { // refacto : do something better
                    //    SmartPanelItemsViewModel::instance->_selectedItem = item;
                    //    summonMessageTextPage();
                    //}
                    break;
                }

                ringTxtBxPlaceHolderDelay("ring id added.", 5000); // refacto : we should check if it's an actual ring id
                contact->ringID_ = Utils::toPlatformString(name);
                contact->_contactStatus = ContactStatus::READY;
                ContactsViewModel::instance->saveContactsToFile();
            }
            else {
                ringTxtBxPlaceHolderDelay("username invalid.", 5000);
                auto item = SmartPanelItemsViewModel::instance->findItem(contact);
                ContactsViewModel::instance->deleteContact(contact);
                SmartPanelItemsViewModel::instance->removeItem(item);
                ContactsViewModel::instance->saveContactsToFile();
            }
            break;
        case LookupStatus::NOT_FOUND:
        {
            ringTxtBxPlaceHolderDelay("username not found.", 5000);
            auto item = SmartPanelItemsViewModel::instance->findItem(contact);
            ContactsViewModel::instance->deleteContact(contact);
            SmartPanelItemsViewModel::instance->removeItem(item);
            ContactsViewModel::instance->saveContactsToFile();
            break;
        }
        case LookupStatus::ERRORR:
            ringTxtBxPlaceHolderDelay("network error!", 5000);
            auto item = SmartPanelItemsViewModel::instance->findItem(contact);
            ContactsViewModel::instance->deleteContact(contact);
            SmartPanelItemsViewModel::instance->removeItem(item);
            ContactsViewModel::instance->saveContactsToFile();
            break;
        }
    }
}


void RingClientUWP::Views::SmartPanel::_RegisterState__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto toggleSwitch = dynamic_cast<ToggleSwitch^>(sender);

    // avoid trouble when InitializeComponent is called.
    if (_usernameTextBox_ == nullptr || _whatWilHappen_ == nullptr)
        return;

    bool isPublic = toggleSwitch->IsOn;

    if (isPublic) {
        RingD::instance->lookUpName(_usernameTextBox_->Text);
        _usernameTextBox_->IsEnabled = true;
        _whatWilHappen_->Text = "peoples will find you with your username.";
    }
    else {
        _usernameTextBox_->IsEnabled = false;
        _whatWilHappen_->Text = "you'll have to send your ringId.";
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    checkStateAddAccountMenu();
}

void RingClientUWP::Views::SmartPanel::_RegisterStateEdition__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto toggleSwitch = dynamic_cast<ToggleSwitch^>(sender);

    // avoid trouble when InitializeComponent is called.
    if (_usernameTextBoxEdition_ == nullptr /*|| _registerOnBlockchainEdition_ == nullptr*/ || _whatWilHappen_ == nullptr)
        return;

    bool isPublic = toggleSwitch->IsOn;

    if (isPublic) {
        RingD::instance->lookUpName(_usernameTextBoxEdition_->Text);
        _usernameTextBoxEdition_->IsEnabled = true;
        //_registerOnBlockchainEdition_->IsEnabled = true;
        _whatWilHappen_->Text = "You are not yet registered.";
    }
    else {
        _usernameTextBoxEdition_->IsEnabled = false;
        // _registerOnBlockchainEdition_->IsEnabled = false;
        _whatWilHappen_->Text = "You'll have to send your ringId.";
        _usernameValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    checkStateEditionMenu();
}


void RingClientUWP::Views::SmartPanel::_usernameTextBox__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    RingD::instance->lookUpName(_usernameTextBox_->Text);

    _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;


}



void RingClientUWP::Views::SmartPanel::_deleteAccountEdition__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto accountType = AccountListItemsViewModel::instance->_selectedItem->_account->accountType_;

    if (accountType=="RING")
        _whatWilHappendeleteRingAccountEdition_->Visibility = (_deleteAccountEdition_->IsOn)
                ? Windows::UI::Xaml::Visibility::Visible
                : Windows::UI::Xaml::Visibility::Collapsed;
    else
        _whatWilHappendeleteSipAccountEdition_->Visibility = (_deleteAccountEdition_->IsOn)
                ? Windows::UI::Xaml::Visibility::Visible
                : Windows::UI::Xaml::Visibility::Collapsed;

    _learnMoreDeleteAccountEdition_->Visibility = (_deleteAccountEdition_->IsOn)
            ? Windows::UI::Xaml::Visibility::Visible
            : Windows::UI::Xaml::Visibility::Collapsed;

    _scrollViewerEditionMenu_->UpdateLayout();
    _scrollViewerEditionMenu_->ScrollToVerticalOffset(_scrollViewerEditionMenu_->ScrollableHeight);
}

// VIDEO

void
SmartPanel::_videoDeviceComboBox__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^)
{
    if (_videoDeviceComboBox_->Items->Size) {
        Video::VideoCaptureManager^ vcm = Video::VideoManager::instance->captureManager();
        auto selectedItem = static_cast<ComboBoxItem^>(static_cast<ComboBox^>(sender)->SelectedItem);
        auto deviceId = static_cast<String^>(selectedItem->Tag);
        std::vector<int>::size_type index;
        for(index = 0; index != vcm->deviceList->Size; index++) {
            auto dev =  vcm->deviceList->GetAt(index);
            if (dev->id() == deviceId) {
                break;
            }
        }
        vcm->activeDevice = vcm->deviceList->GetAt(index);

        populateVideoResolutionSettingsComboBox();
    }
}

void
SmartPanel::_videoResolutionComboBox__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^)
{
    if (_videoResolutionComboBox_->Items->Size) {
        Video::VideoCaptureManager^ vcm = Video::VideoManager::instance->captureManager();
        auto device = vcm->activeDevice;
        auto selectedItem = static_cast<ComboBoxItem^>(static_cast<ComboBox^>(sender)->SelectedItem);
        auto selectedResolution = static_cast<String^>(selectedItem->Tag);
        std::vector<int>::size_type index;
        for(index = 0; index != device->resolutionList()->Size; index++) {
            auto res = device->resolutionList()->GetAt(index);
            if (res->getFriendlyName() == selectedResolution) {
                break;
            }
        }
        vcm->activeDevice->setCurrentResolution( device->resolutionList()->GetAt(index) );
        populateVideoRateSettingsComboBox();
    }
}

void
SmartPanel::_videoRateComboBox__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^)
{
    if (_videoRateComboBox_->Items->Size) {
        Video::VideoCaptureManager^ vcm = Video::VideoManager::instance->captureManager();
        auto resolution = vcm->activeDevice->currentResolution();
        auto selectedItem = static_cast<ComboBoxItem^>(static_cast<ComboBox^>(sender)->SelectedItem);
        unsigned int frame_rate = static_cast<unsigned int>(selectedItem->Tag);
        std::vector<int>::size_type index;
        for(index = 0; index != resolution->rateList()->Size; index++) {
            auto rate = resolution->rateList()->GetAt(index);
            if (rate->value() == frame_rate)
                break;
        }
        vcm->activeDevice->currentResolution()->setActiveRate( resolution->rateList()->GetAt(index) );
        if (vcm->isPreviewing) {
            vcm->CleanupCameraAsync()
            .then([=](task<void> cleanupCameraTask) {
                try {
                    cleanupCameraTask.get();
                    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                        CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
                    {
                        vcm->InitializeCameraAsync(true);
                    }));
                }
                catch (Exception^ e) {
                    WriteException(e);
                }
            });
        }
    }
}

void
SmartPanel::populateVideoDeviceSettingsComboBox()
{
    _videoDeviceComboBox_->Items->Clear();
    auto devices = Video::VideoManager::instance->captureManager()->deviceList;
    int index = 0;
    bool deviceSelected = false;
    for (auto device : devices) {
        ComboBoxItem^ comboBoxItem = ref new ComboBoxItem();
        comboBoxItem->Content = device->name();
        comboBoxItem->Tag = device->id();
        _videoDeviceComboBox_->Items->Append(comboBoxItem);
        if (device->isActive() && !deviceSelected) {
            _videoDeviceComboBox_->SelectedIndex = index;
            deviceSelected = true;
        }
        ++index;
    }
    if (!deviceSelected && devices->Size > 0)
        _videoDeviceComboBox_->SelectedIndex = 0;
}

void
SmartPanel::populateVideoResolutionSettingsComboBox()
{
    _videoResolutionComboBox_->Items->Clear();
    Video::Device^ device = Video::VideoManager::instance->captureManager()->activeDevice;
    std::map<std::string, std::string> settings = DRing::getSettings(Utils::toString(device->name()));
    std::string preferredResolution = settings[Video::Device::PreferenceNames::SIZE];
    auto resolutions = device->resolutionList();
    int index = 0;
    bool resolutionSelected = false;
    for (auto resolution : resolutions) {
        ComboBoxItem^ comboBoxItem = ref new ComboBoxItem();
        comboBoxItem->Content = resolution->getFriendlyName();
        comboBoxItem->Tag = resolution->getFriendlyName();
        _videoResolutionComboBox_->Items->Append(comboBoxItem);
        if (!preferredResolution.compare(Utils::toString(resolution->getFriendlyName())) && !resolutionSelected) {
            _videoResolutionComboBox_->SelectedIndex = index;
            resolutionSelected = true;
        }
        ++index;
    }
    if (!resolutionSelected && resolutions->Size > 0)
        _videoResolutionComboBox_->SelectedIndex = 0;
}

void
SmartPanel::populateVideoRateSettingsComboBox()
{
    _videoRateComboBox_->Items->Clear();
    Video::Device^ device = Video::VideoManager::instance->captureManager()->activeDevice;
    std::map<std::string, std::string> settings = DRing::getSettings(Utils::toString(device->name()));
    std::string preferredRate = settings[Video::Device::PreferenceNames::RATE];
    auto resolution = device->currentResolution();
    int index = 0;
    bool rateSelected = false;
    for (auto rate : resolution->rateList()) {
        ComboBoxItem^ comboBoxItem = ref new ComboBoxItem();
        comboBoxItem->Content = rate->name();
        comboBoxItem->Tag = rate->value();
        _videoRateComboBox_->Items->Append(comboBoxItem);
        if (std::stoi(preferredRate) == rate->value() && !rateSelected) {
            _videoRateComboBox_->SelectedIndex = index;
            rateSelected = true;
        }
        ++index;
    }
    if (!rateSelected && resolution->rateList()->Size > 0)
        _videoRateComboBox_->SelectedIndex = 0;
}



void RingClientUWP::Views::SmartPanel::_ringTxtBx__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::Enter) {
        for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
            if (item->_contact->_name == _ringTxtBx_->Text || item->_contact->ringID_ == _ringTxtBx_->Text) {
                SmartPanelItemsViewModel::instance->_selectedItem = item;
                summonMessageTextPage();
            }
        }

        auto contact = ContactsViewModel::instance->addNewContact(_ringTxtBx_->Text, "", ContactStatus::WAITING_FOR_ACTIVATION);
        RingD::instance->lookUpName(_ringTxtBx_->Text);

        _ringTxtBx_->Text = "";

        for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
            item->_showMe = Windows::UI::Xaml::Visibility::Visible;
        }
        return;
    }

    for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
        auto str1 = Utils::toString(item->_contact->_name);
        auto str2 = Utils::toString(_ringTxtBx_->Text);

        if (str1.find(str2) != std::string::npos)
            item->_showMe = Windows::UI::Xaml::Visibility::Visible;
        else
            item->_showMe = Windows::UI::Xaml::Visibility::Collapsed;
    }

}


void RingClientUWP::Views::SmartPanel::_linkThisDeviceBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountAddMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    showLinkThisDeviceStep1();
}


void RingClientUWP::Views::SmartPanel::_step2button__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _step1Menu_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _step2Menu_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _nextstep_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _addAccountYes_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _addAccountNo_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _PINTextBox_->Text = "";
    _ArchivePassword_->Password = "";
    _response_->Text = "";
}


void RingClientUWP::Views::SmartPanel::_step1button__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    showLinkThisDeviceStep1();
}


void RingClientUWP::Views::SmartPanel::_addAccountNo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _accountsMenuButton_->IsChecked = false;
    _accountsMenuButton__Unchecked(nullptr, nullptr);
}


void RingClientUWP::Views::SmartPanel::_addAccountYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]() {
        RingD::instance->registerThisDevice(_PINTextBox_->Text, _ArchivePassword_->Password);
        _ArchivePassword_->Password = "";
        _PINTextBox_->Text = "";
    }));
}


void RingClientUWP::Views::SmartPanel::OnregistrationStateErrorGeneric(const std::string& accountId)
{
    _response_->Text = "Credentials error or PIN expired.";
}


void RingClientUWP::Views::SmartPanel::_PINTextBox__GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _response_->Text = "";
}


void RingClientUWP::Views::SmartPanel::OnregistrationStateRegistered()
{
    _addAccountNo__Click(nullptr, nullptr);
}

void RingClientUWP::Views::SmartPanel::OncallPlaced(Platform::String ^callId)
{
    SmartPanelItemsViewModel::instance->_selectedItem = nullptr;
}

Object ^ RingClientUWP::Views::ContactStatusNotification::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto contactStatus = static_cast<ContactStatus>(value);

    if (contactStatus == ContactStatus::WAITING_FOR_ACTIVATION)
        return Windows::UI::Xaml::Visibility::Visible;
    else
        return Windows::UI::Xaml::Visibility::Collapsed;
}

Object ^ RingClientUWP::Views::ContactStatusNotification::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::ContactStatusNotification::ContactStatusNotification()
{}


/* if you changed the name of Grid_PointerReleased, be sure to change it in the comment about the selection */
void RingClientUWP::Views::SmartPanel::Grid_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto grid = dynamic_cast<Grid^>(sender);
    auto item = dynamic_cast<SmartPanelItem^>(grid->DataContext);

    if (item) {
        /* if the contact is not yet ready to be used, typically when we are waiting a lookup from the blockachin*/
        auto contact = item->_contact;

        if (contact == nullptr)
        {
            ERR_("SmartPanelIem without contact");
            return;
        }

        if (contact->_contactStatus == ContactStatus::WAITING_FOR_ACTIVATION) {
            return;
        }

        /* if the contact was already selected, just do a deselection and live the message text page*/
        if (item == SmartPanelItemsViewModel::instance->_selectedItem)
        {
            SmartPanelItemsViewModel::instance->_selectedItem = nullptr;
            summonWelcomePage();
            return;
        }

        /* we set the current selected item */
        SmartPanelItemsViewModel::instance->_selectedItem = item;

        /* at this point we check if a call is in progress with the current selected contact*/
        if (item->_callStatus == CallStatus::IN_PROGRESS
                || item->_callStatus == CallStatus::PAUSED
                || item->_callStatus == CallStatus::PEER_PAUSED) {
            if (contact) {
                contact->_unreadMessages = 0;
                ContactsViewModel::instance->saveContactsToFile();
            }

            summonVideoPage();
            return;
        }

        /* else, summont the message text page*/
        summonMessageTextPage();
        contact->_unreadMessages = 0;
        ContactsViewModel::instance->saveContactsToFile();
    }
}

Object ^ RingClientUWP::Views::boolToVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    if (static_cast<bool>(value))
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object ^ RingClientUWP::Views::boolToVisibility::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::boolToVisibility::boolToVisibility()
{}


void RingClientUWP::Views::SmartPanel::OnincomingAccountMessage(Platform::String ^accountId, Platform::String ^from, Platform::String ^payload)
{
    auto contact = ContactsViewModel::instance->findContactByRingId(from);
    auto item = SmartPanelItemsViewModel::instance->findItem(contact);

    /* move the item of the top of the list */
    unsigned int index;
    if (_smartList_->Items->IndexOf(item, &index)) {
        SmartPanelItemsViewModel::instance->moveItemToTheTop(item);
        _smartList_->UpdateLayout();
        _smartList_->ScrollIntoView(item);
    }
}

Object ^ RingClientUWP::Views::CallStatusToSpinnerVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto callStatus = static_cast<CallStatus>(value);

    if (callStatus == CallStatus::INCOMING_RINGING
            || callStatus == CallStatus::OUTGOING_REQUESTED
            || callStatus == CallStatus::OUTGOING_RINGING
            || callStatus == CallStatus::SEARCHING)
        return  Windows::UI::Xaml::Visibility::Visible;
    else
        return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object ^ RingClientUWP::Views::CallStatusToSpinnerVisibility::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::CallStatusToSpinnerVisibility::CallStatusToSpinnerVisibility()
{}

Object ^ RingClientUWP::Views::CallStatusForIncomingCallAnimatedEllipse::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto callStatus = static_cast<CallStatus>(value);

    if (callStatus == CallStatus::INCOMING_RINGING) {
        return  Windows::UI::Xaml::Visibility::Visible;
    }
    else {
        return  Windows::UI::Xaml::Visibility::Collapsed;
    }
}

Object ^ RingClientUWP::Views::CallStatusForIncomingCallAnimatedEllipse::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::CallStatusForIncomingCallAnimatedEllipse::CallStatusForIncomingCallAnimatedEllipse()
{}

Object ^ RingClientUWP::Views::CallStatusForIncomingCallStaticEllipse::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto callStatus = static_cast<CallStatus>(value);

    if (callStatus == CallStatus::INCOMING_RINGING) {
        return  Windows::UI::Xaml::Visibility::Collapsed;
    }
    else {
        return  Windows::UI::Xaml::Visibility::Visible;
    }
}

Object ^ RingClientUWP::Views::CallStatusForIncomingCallStaticEllipse::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::Views::CallStatusForIncomingCallStaticEllipse::CallStatusForIncomingCallStaticEllipse()
{}


void RingClientUWP::Views::SmartPanel::_ringTxtBx__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
        if (item->_contact->_name == _ringTxtBx_->Text || item->_contact->ringID_ == _ringTxtBx_->Text) {
            SmartPanelItemsViewModel::instance->_selectedItem = item;
            summonMessageTextPage();
        }

        auto contact = ContactsViewModel::instance->addNewContact(_ringTxtBx_->Text, "", ContactStatus::WAITING_FOR_ACTIVATION);
        RingD::instance->lookUpName(_ringTxtBx_->Text);

        _ringTxtBx_->Text = "";

        for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
            item->_showMe = Windows::UI::Xaml::Visibility::Visible;
        }
        return;
    }

    for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
        auto str1 = Utils::toString(item->_contact->_name);
        auto str2 = Utils::toString(_ringTxtBx_->Text);

        if (str1.find(str2) != std::string::npos)
            item->_showMe = Windows::UI::Xaml::Visibility::Visible;
        else
            item->_showMe = Windows::UI::Xaml::Visibility::Collapsed;
    }
}
