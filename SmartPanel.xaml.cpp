﻿﻿/***************************************************************************
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
#include "SmartPanel.xaml.h"
#include "qrencode.h"
#include <MemoryBuffer.h>   // IMemoryBufferByteAccess
#include "callmanager_interface.h"
#include "configurationmanager_interface.h"
#include "presencemanager_interface.h"

#include <string>
#include <direct.h>
#include <regex>

#include "lodepng.h"

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
using namespace Windows::UI::Popups;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::DataTransfer;
using namespace Windows::Storage;
using namespace Windows::UI::Core;
using namespace Windows::System::Threading;

using namespace Windows::UI::Xaml::Interop;

SmartPanel::SmartPanel()
{
    InitializeComponent();

    /* populate account list */
    _accountsList_->ItemsSource = AccountListItemsViewModel::instance->itemsList;

    /* populate account's device list */
    // As the user must have the account in question selected before interacting with
    // it's device list, no filtered list is required.
    _devicesIdList_->ItemsSource = RingDeviceItemsViewModel::instance->itemsList;

    /* populate the contact list */
    _smartList_->ItemsSource = SmartPanelItemsViewModel::instance->itemsListFiltered;

    /* populate banned contact list */
    _bannedContactList_->ItemsSource = SmartPanelItemsViewModel::instance->itemsListBannedFiltered;

    /* populate contact request list */
    _incomingContactRequestList_->ItemsSource = ContactRequestItemsViewModel::instance->itemsListFiltered;

    /* connect delegates */
    Configuration::UserPreferences::instance->selectIndex += ref new SelectIndex([&](int index) {
        if (_accountsList_) {
            auto accountsListSize = dynamic_cast<Vector<AccountListItem^>^>(_accountsList_->ItemsSource)->Size;
            if (accountsListSize > index) {
                _accountsList_->SelectedIndex = index;
            }
            else {
                if (accountsListSize > 0)
                    _accountsList_->SelectedIndex = 0;
            }
            updateNotificationsState();
        }
    });

    Configuration::UserPreferences::instance->loadProfileImage += ref new LoadProfileImage([this]() {
        MSG_("LOADING PROFILE IMAGE");
        StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
        String^ image_path = localfolder->Path + "\\.profile\\profile_image.png";
        auto uri = ref new Windows::Foundation::Uri(image_path);
        _selectedAccountAvatar_->ImageSource = ref new BitmapImage(uri);
        Configuration::UserPreferences::instance->profileImageLoaded = true;
    });
    AccountsViewModel::instance->updateScrollView += ref new UpdateScrollView([this]() {
        _accountsListScrollView_->UpdateLayout();
        _accountsListScrollView_->ScrollToVerticalOffset(_accountsListScrollView_->ScrollableHeight);
    });

    RingD::instance->networkChanged += ref new NetworkChanged([&]() {
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
            CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
        {
            _networkConnectivityNotificationRow_->Height = RingD::instance->_hasInternet ? GridLength(0.) : GridLength(32.);
        }));
    });

    RingD::instance->incomingCall += ref new RingClientUWP::IncomingCall([&](
        String^ accountId, String^ callId, String^ from)
    {
        auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountId));
        auto contact = contactListModel->findContactByRingId(from);

		if (contact == nullptr) {
			contact = contactListModel->addNewContact(from, from, TrustStatus::UNKNOWN);
		}

        if (contact == nullptr) {
            return;
        }

        RingD::instance->lookUpAddress(Utils::toString(accountId), from);

        if (auto item = SmartPanelItemsViewModel::instance->findItem(contact)) {
            item->_callId = callId;
            SmartPanelItemsViewModel::instance->moveItemToTheTop(item);
            _smartList_->UpdateLayout();
            _smartList_->ScrollIntoView(item);
        }
    });
    AccountsViewModel::instance->newUnreadMessage += ref new NewUnreadMessage(this, &SmartPanel::updateContactNotificationsState);
    AccountsViewModel::instance->contactDataModified += ref new ContactDataModified(this, &SmartPanel::OncontactDataModified);
    AccountsViewModel::instance->newUnreadContactRequest += ref new NewUnreadContactRequest(this, &SmartPanel::updateNotificationsState);
    RingD::instance->stateChange += ref new StateChange(this, &SmartPanel::OnstateChange);
    RingD::instance->devicesListRefreshed += ref new RingClientUWP::DevicesListRefreshed(this, &RingClientUWP::Views::SmartPanel::OndevicesListRefreshed);
    AccountsViewModel::instance->contactAdded += ref new ContactAdded([this](String^ accountId, Contact^ contact) {
        auto smartPanelItem = ref new SmartPanelItem();
        smartPanelItem->_contact = contact;
        SmartPanelItemsViewModel::instance->itemsList->InsertAt(0, smartPanelItem);
        if (contact->_accountIdAssociated == AccountListItemsViewModel::instance->getSelectedAccountId()) {
            SmartPanelItemsViewModel::instance->itemsListFiltered->InsertAt(0, smartPanelItem);
        }
    });

    RingD::instance->registrationStateRegistered += ref new RingClientUWP::RegistrationStateRegistered(this, &SmartPanel::OnregistrationStateChanged);
    RingD::instance->registrationStateUnregistered += ref new RingClientUWP::RegistrationStateUnregistered(this, &SmartPanel::OnregistrationStateChanged);
    RingD::instance->registrationStateTrying += ref new RingClientUWP::RegistrationStateTrying(this, &SmartPanel::OnregistrationStateChanged);

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

    RingD::instance->newBuddyNotification += ref new RingClientUWP::NewBuddyNotification(this, &RingClientUWP::Views::SmartPanel::OnnewBuddyNotification);

    _networkConnectivityNotificationRow_->Height = Utils::hasInternet() ? 0 : 32;

    selectMenu(MenuOpen::CONTACTS_LIST);
}

void
SmartPanel::OnregistrationStateChanged(const std::string& accountId)
{
    AccountListItemsViewModel::instance->update();
}

void
SmartPanel::OnstateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code)
{
    auto item = SmartPanelItemsViewModel::instance->findItem(callId);

    if (!item) {
        WNG_("item not found");
        return;
    }

    switch (state) {
    case CallStatus::NONE:
    case CallStatus::ENDED:
    {
        auto callsList = DRing::getCallList();
        if (callsList.empty())
            _settingsMenuButton_->Visibility = VIS::Visible;

        break;
    }
    case CallStatus::IN_PROGRESS:
    {
        SmartPanelItemsViewModel::instance->_selectedItem = item;
        _smartList_->SelectedIndex = SmartPanelItemsViewModel::instance->getFilteredIndex(item->_contact);
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
}

void
RingClientUWP::Views::SmartPanel::updatePageContent()
{
    auto accountListItem = AccountListItemsViewModel::instance->_selectedItem;
    if (!accountListItem)
        return;

    auto name = accountListItem->_account->_bestName;

    Configuration::UserPreferences::instance->PREF_ACCOUNT_INDEX = _accountsList_->SelectedIndex;
    Configuration::UserPreferences::instance->save();

    _selectedAccountName_->Text = name;
    _deviceId_->Text = accountListItem->_account->_deviceId;
    _deviceName_->Text = accountListItem->_account->_deviceName;

    _devicesMenuButton_->Visibility = (accountListItem->_account->accountType_ == "RING")
                                      ? Windows::UI::Xaml::Visibility::Visible
                                      : Windows::UI::Xaml::Visibility::Collapsed;

    _shareMenuButton_->Visibility = (accountListItem->_account->accountType_ == "RING")
                                    ? Windows::UI::Xaml::Visibility::Visible
                                    : Windows::UI::Xaml::Visibility::Collapsed;

    _enabledState_->IsOn = accountListItem->_account->_active;
    _upnpState_->IsOn = accountListItem->_account->_upnpState;
    _autoAnswerToggle_->IsOn = accountListItem->_account->_autoAnswer;
    _dhtPublicInCallsToggle_->IsOn = accountListItem->_account->_dhtPublicInCalls;
    _turnEnabledToggle_->IsOn = accountListItem->_account->_turnEnabled;

    if (_RegisterStateEdition_->IsOn) {
        _usernameTextBoxEdition_->IsEnabled = true;
        _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    } else {
        _usernameTextBoxEdition_->IsEnabled = false;
        _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    ContactRequestItemsViewModel::instance->refreshFilteredItemsList();
    ContactRequestItemsViewModel::instance->update(ViewModel::NotifyStrings::notifyContactRequestItem);

    SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
    SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);

    AccountListItemsViewModel::instance->update();
}

void RingClientUWP::Views::SmartPanel::setMode(RingClientUWP::Views::SmartPanel::Mode mode)
{
    _rowRingTxtBx_->Height = (mode == RingClientUWP::Views::SmartPanel::Mode::Normal)? 40 : 0;
    _contactsTitleRow_->Height = (mode == RingClientUWP::Views::SmartPanel::Mode::Normal) ? 30 : 0;
    selectMenu(MenuOpen::CONTACTS_LIST);
}

void RingClientUWP::Views::SmartPanel::_addAccountBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    _accountCreationMenuGrid_->UpdateLayout();
    _accountCreationMenuScrollViewer_->ScrollToVerticalOffset(0);

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

    selectMenu(MenuOpen::ACCOUNTS_LIST);
}


void RingClientUWP::Views::SmartPanel::_createAccountNo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::ACCOUNTS_LIST);
}

/* using the usual selection behaviour doesn't allow us to deselect by simple click. The selection is managed
   by SmartPanelItem_Grid_PointerReleased */
void
SmartPanel::_smartList__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    return;
}

void
SmartPanel::_accountList__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    auto listbox = safe_cast<ListBox^>(sender);
    // disable deselection from listbox
    undoListBoxDeselection(listbox, e);
    auto accountItem = safe_cast<AccountListItem^>(listbox->SelectedItem);
    if (accountItem == nullptr) {
        listbox->SelectedIndex = 0;
        accountItem = safe_cast<AccountListItem^>(listbox->SelectedItem);
    }
    _selectedAccountName_->Text = accountItem->_account->_bestName;
    AccountListItemsViewModel::instance->_selectedItem = accountItem;
    accountItem->_isSelected = true;
    updateNotificationsState();
    Configuration::UserPreferences::instance->PREF_ACCOUNT_INDEX = _accountsList_->SelectedIndex;
    Configuration::UserPreferences::instance->save();
}

void
SmartPanel::undoListBoxDeselection(ListBox^ listBox, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    // disable deselection from listbox
    if (listBox->SelectedItem == nullptr) {
        if (e->RemovedItems->Size > 0) {
            Object^ itemToReselect = e->RemovedItems->GetAt(0);
            for each (auto item in listBox->Items) {
                if (item == itemToReselect) {
                    listBox->SelectedItem = itemToReselect;
                    continue;
                }
            }
        }
    }
}

void RingClientUWP::Views::SmartPanel::_ringTxtBx__Click(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    auto accountId = AccountListItemsViewModel::instance->getSelectedAccountId();
    RingD::instance->lookUpName(Utils::toString(accountId), _ringTxtBx_->Text);
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

            _settingsMenuButton_->Visibility = VIS::Collapsed;

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
        if (item)
            placeCall(item);
    }
}

void
SmartPanel::placeCall(SmartPanelItem^ item)
{
    auto contact = item->_contact;
    if (contact) {
        // select item
        SmartPanelItemsViewModel::instance->_selectedItem = item;
        unsigned index = SmartPanelItemsViewModel::instance->getFilteredIndex(item->_contact);
        _smartList_->SelectedIndex = index;

        for (auto it : SmartPanelItemsViewModel::instance->itemsList)
            if (it->_callStatus == CallStatus::IN_PROGRESS)
                RingD::instance->pauseCall(Utils::toString(it->_callId));

        if (item->_callStatus == CallStatus::ENDED || item->_callStatus == CallStatus::NONE) {
            item->_callStatus = CallStatus::OUTGOING_REQUESTED;
            RingD::instance->placeCall(contact);
            auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
            auto lookingForString = loader->GetString("_callsLookingFor_");
            item->_contact->_lastTime = lookingForString + item->_contact->_name + ".";
        }

        /* move the item of the top of the list */
        if (_smartList_->Items->IndexOf(item, &index)) {
            SmartPanelItemsViewModel::instance->moveItemToTheTop(item);
            _smartList_->UpdateLayout();
            _smartList_->ScrollIntoView(item);
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

void
SmartPanel::SmartPanelItem_Grid_PointerEntered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto grid = dynamic_cast<Grid^>(sender);
    auto item = dynamic_cast<SmartPanelItem^>(grid->DataContext);

    for (auto it : SmartPanelItemsViewModel::instance->itemsList) {
        it->_isHovered = false;
    }

    item->_isHovered = true;
}

void
SmartPanel::SmartPanelItem_Grid_PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto grid = dynamic_cast<Grid^>(sender);
    auto item = dynamic_cast<SmartPanelItem^>(grid->DataContext);

    // to avoid visual bug, do it on the whole list
    for each (auto it in SmartPanelItemsViewModel::instance->itemsList) {
        it->_isHovered = false;
    }
}

void
SmartPanel::generateQRcode()
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
            } else {
                _acceptAccountModification_->IsEnabled = false;
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

void RingClientUWP::Views::SmartPanel::_addDevice__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _devicesMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _closePin_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}


void RingClientUWP::Views::SmartPanel::OndevicesListRefreshed(Map<String^, String^>^ deviceMap)
{
    if (!AccountListItemsViewModel::instance->_selectedItem)
        return;

    _waitingDevicesList_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    RingDeviceItemsViewModel::instance->itemsList->Clear();

    for each (auto device in deviceMap) {
        if (AccountListItemsViewModel::instance->_selectedItem->_account->_deviceId == device->Key) {
            AccountListItemsViewModel::instance->_selectedItem->_account->_deviceName == device->Value;
            _deviceName_->Text = device->Value;
        }
        else {
            RingDeviceItemsViewModel::instance->itemsList->Append(ref new RingDeviceItem(device->Key, device->Value));
        }
    }

    if (deviceMap->Size < 2) {
        _noDevicesList_->Visibility = VIS::Visible;
        _devicesIdList_->Visibility = VIS::Collapsed;
    }
    else {
        _noDevicesList_->Visibility = VIS::Collapsed;
        _devicesIdList_->Visibility = VIS::Visible;
    }
}


void RingClientUWP::Views::SmartPanel::_pinGeneratorYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    requestPin();
}


void RingClientUWP::Views::SmartPanel::_pinGeneratorNo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _devicesMenuButton_->IsChecked = false;
    _passwordForPinGenerator_->Password = "";

    selectMenu(MenuOpen::CONTACTS_LIST);
}


void RingClientUWP::Views::SmartPanel::OnexportOnRingEnded(Platform::String ^accountId, Platform::String ^pin)
{
    _waitingAndResult_->Text = pin;
    _closePin_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    auto frame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Window::Current->Content);
    auto mainPage = dynamic_cast<RingClientUWP::MainPage^>(frame->Content);
    mainPage->showLoadingOverlay(false, false);
    mainPage->setLoadingStatusText("PIN generated", "#ff00ff00");
}


void RingClientUWP::Views::SmartPanel::_closePin__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::CONTACTS_LIST);

    // refacto : do something better...
    auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
    _waitingAndResult_->Text = loader->GetString("_accountsWaitingAndResult_.Text");
}


void RingClientUWP::Views::SmartPanel::_shareMenuDone__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::CONTACTS_LIST);
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
    if ((bool)value == true)
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
    _scrollViewerEditionMenu_->UpdateLayout();
    _scrollViewerEditionMenu_->ScrollToVerticalOffset(0);

    auto account = AccountListItemsViewModel::instance->_selectedItem->_account;

    _accountAliasTextBoxEdition_->Text = account->name_;
    _accountEditionGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _deleteAccountEdition_->IsOn = false;
    _deleteAccountEdition_->IsEnabled = (AccountListItemsViewModel::instance->itemsList->Size > 1)? true : false;
    _createAccountYes_->IsEnabled = false;

    _whatWillHappendeleteRingAccountEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _whatWillHappendeleteSipAccountEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

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

        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _whatWillHappenEdition_->Text = loader->GetString("_whatWillHappen_0_");
        _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    } else {
        _RegisterStateEdition_->IsOn = false;

        _usernameTextBoxEdition_->IsEnabled = false;
        _usernameTextBoxEdition_->Text = "";
        _RegisterStateEdition_->IsEnabled = true;

        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _whatWillHappenEdition_->Text = loader->GetString("_whatWillHappen_1_");
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
    if (_deleteAccountEdition_->IsOn && accountsListSize > 1) {
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
            account->_active = _enabledState_->IsOn;
            account->_upnpState = _upnpState_->IsOn;
            account->_autoAnswer = _autoAnswerToggle_->IsOn;
            account->_dhtPublicInCalls = _dhtPublicInCallsToggle_->IsOn;
            account->_turnEnabled = _turnEnabledToggle_->IsOn;
        }
        else {
            account->_sipHostname = _sipHostnameEdition_->Text;
            account->_sipUsername = _sipUsernameEditionTextBox_->Text;
            account->_sipPassword = _sipPasswordEdition_->Password;
        }

        RingD::instance->updateAccount(accountId);
    }

    selectMenu(MenuOpen::ACCOUNTS_LIST);

    updatePageContent();

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
    create_task(Configuration::getProfileImageAsync()).then([&](task<BitmapImage^> image){
        try {
            if (auto bitmapImage = image.get()) {
                _selectedAccountAvatar_->ImageSource = bitmapImage;
            }
        }
        catch (Platform::Exception^ e) {
            EXC_(e);
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

void
SmartPanel::SmartPanelItem_Grid_PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
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
    auto accountId = AccountListItemsViewModel::instance->getSelectedAccountId();
    RingD::instance->lookUpName(Utils::toString(accountId), _usernameTextBoxEdition_->Text);

    _usernameValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    checkStateEditionMenu();
}

void RingClientUWP::Views::SmartPanel::OnregisteredNameFound(RingClientUWP::LookupStatus status, const std::string& accountId, const std::string& address, const std::string& name)
{
    // In the case where the lookup was for a local account's username, we need just
    // update the account's username property and return.
    if (status == LookupStatus::SUCCESS) {
        if (Account^ account = AccountsViewModel::instance->findAccountByRingID(Utils::toPlatformString(address))) {
            MSG_("Account username lookup complete");
            account->_username = Utils::toPlatformString(name);
            return;
        }
    }

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
        //auto selectedAccountId = AccountListItemsViewModel::instance->getSelectedAccountId();
        auto contactListModel = AccountsViewModel::instance->getContactListModel(std::string(accountId));
        auto contact = contactListModel->findContactByName(Utils::toPlatformString(name));

        // Try looking up a contact added by address
        if (contact == nullptr)
            contact = contactListModel->findContactByName(Utils::toPlatformString(address));

        if (contact == nullptr)
            return;

        switch (status) {
        case LookupStatus::SUCCESS:
        {
            if (contact->_contactStatus == ContactStatus::WAITING_FOR_ACTIVATION) {
                contact->_contactStatus = ContactStatus::READY;
                contact->ringID_ = Utils::toPlatformString(address);
                auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
                ringTxtBxPlaceHolderDelay(loader->GetString("_contactsUserAdded_"), 5000);

                // send contact request
                if (contact->_trustStatus == TrustStatus::UNKNOWN) {
                    auto vcard = Configuration::UserPreferences::instance->getVCard();
                    RingD::instance->sendContactRequest(accountId, address, vcard->asString());
                    contact->_name = Utils::toPlatformString(name);
                    contact->_trustStatus = TrustStatus::CONTACT_REQUEST_SENT;
                    SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                    SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);
                }
                else if (contact->_trustStatus == TrustStatus::INCOMING_CONTACT_REQUEST) {
                    contact->_name = Utils::toPlatformString(name);
                    ContactRequestItemsViewModel::instance->update(ViewModel::NotifyStrings::notifyContactRequestItem);
                }
                if (contact && !contact->subscribed_) {
                        MSG_("account: " + accountId + " subscribing to buddy presence for ringID: " + Utils::toString(contact->ringID_));
                        RingD::instance->subscribeBuddy(accountId, Utils::toString(contact->ringID_), true);
                        contact->subscribed_ = true;
                }
                contactListModel->saveContactsToFile();
            }
            else {
                /* in that case we delete a possible suroggate */
                for each (Contact^ co in contactListModel->_contactsList) {
                    if (co->_contactStatus == ContactStatus::WAITING_FOR_ACTIVATION
                            && co->_name == Utils::toPlatformString(name)) {
                        auto item = SmartPanelItemsViewModel::instance->findItem(co);
                        contactListModel->deleteContact(co);
                        SmartPanelItemsViewModel::instance->removeItem(item);
                    }
                }
            }
        }
        break;
        case LookupStatus::INVALID_NAME:
        {
            MSG_("INVALID_NAME LOOKUP RESULT");
            std::regex sha1_regex("[0-9a-f]{40}");
            if (std::regex_match(name, sha1_regex)) {
                /* first we check if some contact is registred with this ring id */
                auto contactAlreadyRecorded = contactListModel->findContactByRingId(Utils::toPlatformString(name));
                if (contactAlreadyRecorded) {
                    /* delete the contact added recently */
                    auto item = SmartPanelItemsViewModel::instance->findItem(contact);
                    if (item->_contact->_trustStatus != TrustStatus::INCOMING_CONTACT_REQUEST &&
                        item->_contact->_trustStatus != TrustStatus::UNKNOWN &&
                        item->_contact->_contactStatus == ContactStatus::WAITING_FOR_ACTIVATION) {
                        contactListModel->deleteContact(contact);
                        SmartPanelItemsViewModel::instance->removeItem(item);
                        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
                        ringTxtBxPlaceHolderDelay(loader->GetString("_contactsContactExists_"), 5000);
                    }
                    else if (item->_contact->_trustStatus == TrustStatus::UNKNOWN) {
                        SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                        SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);
                    }
                }
                else {
                    auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
                    ringTxtBxPlaceHolderDelay(loader->GetString("_contactsRingIdAdded_"), 5000);
                    contact->ringID_ = Utils::toPlatformString(name);
                    contact->_contactStatus = ContactStatus::READY;

                    if (contact && !contact->subscribed_) {
                        MSG_("account: " + accountId + " subscribing to buddy presence for ringID: " + Utils::toString(contact->ringID_));
                        RingD::instance->subscribeBuddy(accountId, Utils::toString(contact->ringID_), true);
                        contact->subscribed_ = true;
                    }

                    // send contact request
                    if (contact->_trustStatus == TrustStatus::UNKNOWN) {
                        auto vcard = Configuration::UserPreferences::instance->getVCard();
                        RingD::instance->sendContactRequest(accountId, address, vcard->asString());
                        contact->_trustStatus = TrustStatus::CONTACT_REQUEST_SENT;
                        SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                        SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);
                    }
                }
            }
            else {
                auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
                ringTxtBxPlaceHolderDelay(loader->GetString("_contactsUsernameInvalid_"), 5000);
                auto item = SmartPanelItemsViewModel::instance->findItem(contact);
                contactListModel->deleteContact(contact);
                SmartPanelItemsViewModel::instance->removeItem(item);
            }
            contactListModel->saveContactsToFile();
            break;
        }
        case LookupStatus::NOT_FOUND:
        {
            auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
            ringTxtBxPlaceHolderDelay(loader->GetString("_contactsUsernameNotFound_"), 5000);
            auto item = SmartPanelItemsViewModel::instance->findItem(contact);
            contactListModel->deleteContact(contact);
            SmartPanelItemsViewModel::instance->removeItem(item);
            contactListModel->saveContactsToFile();
            break;
        }
        case LookupStatus::ERRORR:
            std::regex sha1_regex("[0-9a-f]{40}");
            if (std::regex_match(address, sha1_regex)) {
                RingD::instance->lookUpName(accountId, Utils::toPlatformString(address));
            }
            else {
                auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
                ringTxtBxPlaceHolderDelay(loader->GetString("_contactsNetworkError_"), 5000);
                auto item = SmartPanelItemsViewModel::instance->findItem(contact);
                contactListModel->deleteContact(contact);
                SmartPanelItemsViewModel::instance->removeItem(item);
                contactListModel->saveContactsToFile();
            }
            break;
        }
    }
}


void RingClientUWP::Views::SmartPanel::_RegisterState__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto toggleSwitch = dynamic_cast<ToggleSwitch^>(sender);

    // avoid trouble when InitializeComponent is called.
    if (_usernameTextBox_ == nullptr || _whatWillHappen_ == nullptr)
        return;

    bool isPublic = toggleSwitch->IsOn;

    if (isPublic) {
        auto accountId = AccountListItemsViewModel::instance->getSelectedAccountId();
        RingD::instance->lookUpName(Utils::toString(accountId), _usernameTextBox_->Text);
        _usernameTextBox_->IsEnabled = true;
        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _whatWillHappen_->Text = loader->GetString("_whatWillHappen_0_");
    }
    else {
        _usernameTextBox_->IsEnabled = false;
        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _whatWillHappen_->Text = loader->GetString("_whatWillHappen_2_");
        _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    checkStateAddAccountMenu();
}

void RingClientUWP::Views::SmartPanel::_RegisterStateEdition__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto toggleSwitch = dynamic_cast<ToggleSwitch^>(sender);

    // avoid trouble when InitializeComponent is called.
    if (_usernameTextBoxEdition_ == nullptr || _whatWillHappen_ == nullptr)
        return;

    bool isPublic = toggleSwitch->IsOn;

    if (isPublic) {
        auto accountId = AccountListItemsViewModel::instance->getSelectedAccountId();
        RingD::instance->lookUpName(Utils::toString(accountId), _usernameTextBoxEdition_->Text);
        _usernameTextBoxEdition_->IsEnabled = true;
        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _whatWillHappen_->Text = loader->GetString("_whatWillHappen_1_");
    }
    else {
        _usernameTextBoxEdition_->IsEnabled = false;
        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _whatWillHappen_->Text = loader->GetString("_whatWillHappen_2_");
        _usernameValidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _usernameInvalidEdition_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }

    checkStateEditionMenu();
}


void RingClientUWP::Views::SmartPanel::_usernameTextBox__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    auto accountId = AccountListItemsViewModel::instance->getSelectedAccountId();
    RingD::instance->lookUpName(Utils::toString(accountId), _usernameTextBox_->Text);

    _usernameValid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _usernameInvalid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}



void RingClientUWP::Views::SmartPanel::_deleteAccountEdition__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto accountType = AccountListItemsViewModel::instance->_selectedItem->_account->accountType_;

    if (accountType=="RING")
        _whatWillHappendeleteRingAccountEdition_->Visibility = (_deleteAccountEdition_->IsOn)
                ? Windows::UI::Xaml::Visibility::Visible
                : Windows::UI::Xaml::Visibility::Collapsed;
    else
        _whatWillHappendeleteSipAccountEdition_->Visibility = (_deleteAccountEdition_->IsOn)
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
                    EXC_(e);
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

    _addAccountYes_->IsEnabled = false;
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
    selectMenu(MenuOpen::CONTACTS_LIST);
}


void RingClientUWP::Views::SmartPanel::_addAccountYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::ACCOUNTS_LIST);
    this->Dispatcher->RunAsync(Windows::UI::Core::CoreDispatcherPriority::Normal, ref new Windows::UI::Core::DispatchedHandler([this]() {
        auto frame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^ > (Window::Current->Content);
        dynamic_cast<RingClientUWP::MainPage^>(frame->Content)->showLoadingOverlay(true, true);
        dynamic_cast<RingClientUWP::MainPage^>(frame->Content)->setLoadingStatusText("Attempting to add this device...", "#ff00ff00");
        RingD::instance->registerThisDevice(_PINTextBox_->Text, _ArchivePassword_->Password);
        _ArchivePassword_->Password = "";
        _PINTextBox_->Text = "";
    }));
}


void RingClientUWP::Views::SmartPanel::OnregistrationStateErrorGeneric(const std::string& accountId)
{
    auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
    _response_->Text = loader->GetString("_accountsCredentialsExpired_");
}


void RingClientUWP::Views::SmartPanel::_PINTextBox__GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _response_->Text = "";
}


void RingClientUWP::Views::SmartPanel::OnregistrationStateRegistered(const std::string& accountId)
{
}

void RingClientUWP::Views::SmartPanel::OncallPlaced(Platform::String ^callId)
{
    SmartPanelItemsViewModel::instance->_selectedItem = nullptr;
}

void RingClientUWP::Views::SmartPanel::OncontactDataModified(Platform::String ^account, Contact^ contact)
{
    updateNotificationsState();
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


void RingClientUWP::Views::SmartPanel::selectMenu(MenuOpen menu)
{
    _contactsListMenuButton_->IsChecked = (menu == MenuOpen::CONTACTS_LIST) ? true : false;
    _smartGrid_->Visibility = (menu == MenuOpen::CONTACTS_LIST) ? VIS::Visible : VIS::Collapsed;

    _contactRequestListMenuButton_->IsChecked = (menu == MenuOpen::CONTACTREQUEST_LIST) ? true : false;
    _incomingContactRequestGrid_->Visibility = (menu == MenuOpen::CONTACTREQUEST_LIST) ? VIS::Visible : VIS::Collapsed;

    _accountsMenuButton_->IsChecked = (menu == MenuOpen::ACCOUNTS_LIST) ? true : false;
    _accountsMenuGrid_->Visibility = (menu == MenuOpen::ACCOUNTS_LIST) ? VIS::Visible : VIS::Collapsed;

    _shareMenuButton_->IsChecked = (menu == MenuOpen::SHARE) ? true : false;
    _shareMenuGrid_->Visibility = (menu == MenuOpen::SHARE) ? VIS::Visible : VIS::Collapsed;

    _devicesMenuButton_->IsChecked = (menu == MenuOpen::DEVICE) ? true : false;
    _devicesMenuGrid_->Visibility = (menu == MenuOpen::DEVICE) ? VIS::Visible : VIS::Collapsed;

    _settingsMenuButton_->IsChecked = (menu == MenuOpen::SETTINGS) ? true : false;
    _settingsMenu_->Visibility = (menu == MenuOpen::SETTINGS) ? VIS::Visible : VIS::Collapsed;

    /* manage the account menu */
    _accountCreationMenuGrid_->Visibility = VIS::Collapsed;
    _accountEditionGrid_->Visibility = VIS::Collapsed;
    _accountAddMenuGrid_->Visibility = VIS::Collapsed;

    if (menu == MenuOpen::CONTACTREQUEST_LIST) {
        _incomingContactRequestList_->SelectedIndex = -1;
        ContactRequestItemsViewModel::instance->_selectedItem = nullptr;
    }

    /* manage the share icon*/
    if (menu == MenuOpen::SHARE && menuOpen != MenuOpen::SHARE) {
        generateQRcode();
        _qrCodeIconWhite_->Visibility = VIS::Collapsed;
        _qrCodeIconBlack_->Visibility = VIS::Visible;
    }
    else if (menu != MenuOpen::SHARE && menuOpen == MenuOpen::SHARE) {
        _qrCodeIconBlack_->Visibility = VIS::Collapsed;
        _qrCodeIconWhite_->Visibility = VIS::Visible;
    }

    /* manage adding device menu */
    _addingDeviceGrid_->Visibility = VIS::Collapsed;
    _waitingDevicesList_->Visibility = VIS::Collapsed;
    _waitingForPin_->Visibility = VIS::Collapsed;

    /* manage the video preview */
    if (menu == MenuOpen::SETTINGS && menuOpen != MenuOpen::SETTINGS) {
        auto vcm = Video::VideoManager::instance->captureManager();
        if (vcm->deviceList->Size > 0) {
            if (!vcm->isInitialized)
                vcm->InitializeCameraAsync(true);
            else
                vcm->StartPreviewAsync(true);
        }
        summonPreviewPage();
    }
    else if (menu != MenuOpen::SETTINGS && menuOpen == MenuOpen::SETTINGS) {
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
                    EXC_(e);
                }
            });
        }
        hidePreviewPage();
    }

    menuOpen = menu;
}

void
SmartPanel::_addAccountInputValidation__KeyUp(Platform::Object ^ sender, RoutedEventArgs ^ e)
{
    bool isPasswordValid = (!_PINTextBox_->Text->IsEmpty()) && (!_ArchivePassword_->Password->IsEmpty()) ? true : false;
    _addAccountYes_->IsEnabled = isPasswordValid;
}

/* if you changed the name of SmartPanelItem_Grid_PointerReleased, be sure to change it in the comment about the selection */
void
SmartPanel::SmartPanelItem_Grid_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto grid = dynamic_cast<Grid^>(sender);
    auto item = dynamic_cast<SmartPanelItem^>(grid->DataContext);

    if (!item)
        return;

    /* if the contact is not yet ready to be used, typically when we are waiting a lookup from the blockachin*/
    auto contact = item->_contact;

    if (contact == nullptr)
    {
        ERR_("SmartPanelItem without contact");
        return;
    }

    if (contact->_contactStatus == ContactStatus::WAITING_FOR_ACTIVATION) {
        return;
    }

    /* we set the current selected item */
    if (SmartPanelItemsViewModel::instance->_selectedItem != item) {
        SmartPanelItemsViewModel::instance->_selectedItem = item;

        /* at this point we check if a call is in progress with the current selected contact*/
        auto selectedAccountId = AccountListItemsViewModel::instance->getSelectedAccountId();
        auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(selectedAccountId));
        // TODO: don't clear messages if the user does not see the message panel from the VideoPage
        if (contact) {
            contact->_unreadMessages = 0;
            updateNotificationsState();
            contactListModel->saveContactsToFile();
        }
    }

    /* summon the video when in a call */
    if (item->_callStatus == CallStatus::IN_PROGRESS
        || item->_callStatus == CallStatus::PAUSED
        || item->_callStatus == CallStatus::PEER_PAUSED) {
        summonVideoPage();
        return;
    }

    /* else, summon the message text page*/
    summonMessageTextPage();
}

void
SmartPanel::updateNotificationsState()
{
    updateContactNotificationsState(nullptr);
}

void
SmartPanel::updateContactNotificationsState(Contact^ contact)
{
    if (contact != nullptr) {
        auto smartListIndex = SmartPanelItemsViewModel::instance->getIndex(contact);
        auto smartPanelItem = SmartPanelItemsViewModel::instance->findItem(contact);
        auto selectedSmartPanelItem = SmartPanelItemsViewModel::instance->_selectedItem;
        auto frame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Window::Current->Content);
        auto mainPage = dynamic_cast<RingClientUWP::MainPage^>(frame->Content);
        if (_smartList_->SelectedIndex != smartListIndex &&
            !(selectedSmartPanelItem == smartPanelItem && mainPage->currentFrame == FrameOpen::MESSAGE)) {
            contact->_unreadMessages++;
            /* saveContactsToFile used to save the notification */
            auto selectedAccountId = AccountListItemsViewModel::instance->getSelectedAccountId();
            auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(selectedAccountId));
            contactListModel->saveContactsToFile();
        }
    }

    auto selectedAccountId = Utils::toString(AccountListItemsViewModel::instance->getSelectedAccountId());
    if (!selectedAccountId.empty()) {

        auto selectedAccountMessages = AccountsViewModel::instance->unreadMessages(Utils::toPlatformString(selectedAccountId));
        auto selectedAccountContactRequests = AccountsViewModel::instance->unreadContactRequests(Utils::toPlatformString(selectedAccountId));

        _unreadMessagesCircle_->Visibility = selectedAccountMessages ? VIS::Visible : VIS::Collapsed;
        _unreadContactRequestsCircle_->Visibility = selectedAccountContactRequests ? VIS::Visible : VIS::Collapsed;

        auto totalMessages = AccountListItemsViewModel::instance->unreadMessages();
        auto totalContactRequests = AccountListItemsViewModel::instance->unreadContactRequests();

        auto totalNonSelectedNotifications = (totalMessages + totalContactRequests) - (selectedAccountMessages + selectedAccountContactRequests);
        _unreadAccountNotificationsCircle_->Visibility = (totalNonSelectedNotifications > 0) ? VIS::Visible : VIS::Collapsed;
    }

    ContactRequestItemsViewModel::instance->refreshFilteredItemsList();
    ContactRequestItemsViewModel::instance->update(ViewModel::NotifyStrings::notifyContactRequestItem);

    SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
    SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);

    AccountListItemsViewModel::instance->update();
}

Object ^ RingClientUWP::Views::boolToVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto direction = static_cast<String^>(parameter);
    Visibility if_true = (direction == "Inverted") ? Visibility::Collapsed : Visibility::Visible;
    Visibility if_false = (direction == "Inverted") ? Visibility::Visible : Visibility::Collapsed;
    if (static_cast<bool>(value))
        return if_true;
    return  if_false;
}

Object ^ RingClientUWP::Views::boolToVisibility::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

Object^
uintToVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    if (static_cast<unsigned>(value))
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
OneToVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    if (static_cast<unsigned>(value) == 1)
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
UnreadAccountNotificationsString::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto account = static_cast<Account^>(value);
    String^ notificationString;
    std::string notification_string;
    std::string description;
    if (account->_unreadMessages) {
        description = account->_unreadMessages==1 ? " Message" : " Messages";
        notification_string.append(std::to_string(account->_unreadMessages) + description);
    }
    if (account->_unreadContactRequests) {
        if (account->_unreadMessages)
            notification_string.append(", ");
        description = account->_unreadContactRequests == 1 ? " Contact request" : " Contact requests";
        notification_string.append(std::to_string(account->_unreadContactRequests) + description);
    }
    return Utils::toPlatformString(notification_string);
}

Object^
MoreThanOneToVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    if (static_cast<unsigned>(value) > 1)
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
PartialTrustToVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto direction = static_cast<String^>(parameter);
    Visibility if_true = (direction == "Inverted") ? Visibility::Collapsed : Visibility::Visible;
    Visibility if_false = (direction == "Inverted") ? Visibility::Visible : Visibility::Collapsed;
    if (static_cast<TrustStatus>(value) == TrustStatus::CONTACT_REQUEST_SENT)
        return if_true;
    return  if_false;
}

Object^
TrustedToVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    if (static_cast<Contact^>(value)->_isTrusted)
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
SelectedAccountToVisibility::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    auto contact = static_cast<Contact^>(value);
    auto callStatus = SmartPanelItemsViewModel::instance->findItem(contact)->_callStatus;
    auto isCall = ( callStatus != CallStatus::NONE && callStatus != CallStatus::ENDED ) ? true : false;

    if (contact->_accountIdAssociated == AccountListItemsViewModel::instance->getSelectedAccountId() || isCall)
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

void RingClientUWP::Views::SmartPanel::OnincomingAccountMessage(Platform::String ^accountId, Platform::String ^from, Platform::String ^payload)
{
    auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountId));
    auto contact = contactListModel->findContactByRingId(from);
    auto item = SmartPanelItemsViewModel::instance->findItem(contact);

    /* move the item of the top of the list */
    unsigned int index;
    if (_smartList_->Items->IndexOf(item, &index)) {
        SmartPanelItemsViewModel::instance->moveItemToTheTop(item);
        _smartList_->UpdateLayout();
        _smartList_->ScrollIntoView(item);
    }

    auto selectedAccountId = Utils::toString(AccountListItemsViewModel::instance->getSelectedAccountId());
    if (!selectedAccountId.empty()) {
        auto selectedAccountMessages = AccountsViewModel::instance->unreadMessages(Utils::toPlatformString(selectedAccountId));
        auto selectedAccountContactRequests = AccountsViewModel::instance->unreadContactRequests(Utils::toPlatformString(selectedAccountId));

        auto totalMessages = AccountListItemsViewModel::instance->unreadMessages();
        auto totalContactRequests = AccountListItemsViewModel::instance->unreadContactRequests();

        auto totalNonSelectedNotifications = (totalMessages + totalContactRequests) - (selectedAccountMessages + selectedAccountContactRequests);
        _unreadAccountNotificationsCircle_->Visibility = (totalNonSelectedNotifications > 0) ? VIS::Visible : VIS::Collapsed;
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

void
SmartPanel::addToContactList(String^ name)
{
    auto selectedAccountId = AccountListItemsViewModel::instance->getSelectedAccountId();

    auto account = AccountsViewModel::instance->findItem(selectedAccountId);
    auto hasInternet = Utils::hasInternet();
    if (account->accountType_ == "RING" && (!hasInternet || account->_registrationState != RegistrationState::REGISTERED)) {
        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        ringTxtBxPlaceHolderDelay(loader->GetString("_accountNotRegistered_"), 5000);
        return;
    }

    auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(selectedAccountId));
    for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
        if ((item->_contact->_name == name || item->_contact->ringID_ == name) &&
            selectedAccountId == item->_contact->_accountIdAssociated &&
            item->_contact->_trustStatus >= TrustStatus::CONTACT_REQUEST_SENT) {
            SmartPanelItemsViewModel::instance->_selectedItem = item;
            summonMessageTextPage();
            return;
        }
        else if ((item->_contact->_name == name || item->_contact->ringID_ == name) &&
                 item->_contact->_trustStatus == TrustStatus::INCOMING_CONTACT_REQUEST) {
            // In this case, we are potentially attempting to send a trust request to a
            // peer who has already sent us one. For now we shall just send a trust request,
            // so they receive our vcard and remove our corresponding contact request control item.
            /*DRing::acceptTrustRequest(  Utils::toString(item->_contact->_accountIdAssociated),
                                        Utils::toString(item->_contact->ringID_));*/
            auto vcard = Configuration::UserPreferences::instance->getVCard();
            RingD::instance->sendContactRequest(Utils::toString(item->_contact->_accountIdAssociated),
                                                Utils::toString(item->_contact->ringID_),
                                                vcard->asString());
            item->_contact->_trustStatus = TrustStatus::TRUSTED;
            contactListModel->saveContactsToFile();

            auto spi = SmartPanelItemsViewModel::instance->findItem(item->_contact);
            SmartPanelItemsViewModel::instance->moveItemToTheTop(spi);
            SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
            SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);

            if (auto cri = ContactRequestItemsViewModel::instance->findItem(item->_contact))
                ContactRequestItemsViewModel::instance->removeItem(cri);

            ContactRequestItemsViewModel::instance->refreshFilteredItemsList();
            ContactRequestItemsViewModel::instance->update(ViewModel::NotifyStrings::notifyContactRequestItem);

            MSG_("Auto-accepted Contact Request from: " + item->_contact->ringID_);
            return;
        }
    }

    auto contact = contactListModel->addNewContact(_ringTxtBx_->Text, "", TrustStatus::UNKNOWN, ContactStatus::WAITING_FOR_ACTIVATION);

    std::regex sha1_regex("[0-9a-f]{40}");
    if(std::regex_match(Utils::toString(_ringTxtBx_->Text), sha1_regex))
        RingD::instance->lookUpAddress(Utils::toString(selectedAccountId), name);
    else
        RingD::instance->lookUpName(Utils::toString(selectedAccountId), name);

    for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
        item->_isVisible = Windows::UI::Xaml::Visibility::Visible;
    }
}

void RingClientUWP::Views::SmartPanel::_ringTxtBx__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    addToContactList(_ringTxtBx_->Text);
    _ringTxtBx_->Text = "";
}

void RingClientUWP::Views::SmartPanel::_ringTxtBx__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::Enter) {
        addToContactList(_ringTxtBx_->Text);
        _ringTxtBx_->Text = "";
    }
}

void RingClientUWP::Views::SmartPanel::_contactsListMenuButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::CONTACTS_LIST);
}

void RingClientUWP::Views::SmartPanel::_contactRequestListMenuButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::CONTACTREQUEST_LIST);
}

void RingClientUWP::Views::SmartPanel::_accountsMenuButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::ACCOUNTS_LIST);
}

void RingClientUWP::Views::SmartPanel::_shareMenuButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::SHARE);
}

void RingClientUWP::Views::SmartPanel::_devicesMenuButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::DEVICE);

    _pinGeneratorYes_->IsEnabled = false;
    _passwordForPinGenerator_->Password = "";
    // refacto : do something better...
    auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
    _waitingAndResult_->Text = loader->GetString("_accountsWaitingAndResult_.Text");

    _waitingDevicesList_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _devicesIdList_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    auto account = AccountListItemsViewModel::instance->_selectedItem->_account;

    _deviceId_->Text = account->_deviceId;
    _deviceName_->Text = account->_deviceName;
    if (_deviceId_->Text->IsEmpty()) {
        _deviceId_->Text = loader->GetString("_accountsNoDeviceId_");
        ERR_("device Id not found for account " + Utils::toString(account->_deviceId));
    }

    RingD::instance->getKnownDevices(account->accountID_);
}

void
SmartPanel::_homeButton__Click(Platform::Object^ sender, RoutedEventArgs^ e)
{
    if (menuOpen == MenuOpen::SETTINGS)
        selectMenu(MenuOpen::CONTACTS_LIST);

    // de-select item from Listbox only
    _smartList_->SelectedIndex = -1;

    MSG_(_smartList_->SelectedIndex.ToString());
    if (SmartPanelItemsViewModel::instance->_selectedItem)
        MSG_("item selected");
    else
        MSG_("null");

    summonWelcomePage();
}

void RingClientUWP::Views::SmartPanel::_settingsMenuButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    selectMenu(MenuOpen::SETTINGS);
}

void RingClientUWP::Views::SmartPanel::_passwordForPinGenerator__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    bool isPasswordValid = (_passwordForPinGenerator_->Password->IsEmpty()) ? false : true;
    _pinGeneratorYes_->IsEnabled = isPasswordValid;

    if (e->Key == Windows::System::VirtualKey::Enter && isPasswordValid)
        requestPin();
}

void RingClientUWP::Views::SmartPanel::requestPin()
{
    auto frame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Window::Current->Content);
    auto mainPage = dynamic_cast<RingClientUWP::MainPage^>(frame->Content);
    mainPage->showLoadingOverlay(true, true);
    mainPage->setLoadingStatusText("Generating PIN", "#ff00ff00");

    _addingDeviceGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _waitingForPin_->Visibility = Windows::UI::Xaml::Visibility::Visible;

    auto accountId = AccountListItemsViewModel::instance->_selectedItem->_account->accountID_;
    auto password = _passwordForPinGenerator_->Password;
    _passwordForPinGenerator_->Password = "";

    /* hide the button while we are waiting... */
    _closePin_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;

    RingD::instance->askToExportOnRing(accountId, password);
}

void
SmartPanel::ContactRequestItem_Grid_PointerReleased(Platform::Object ^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ e)
{
    return;
}

void
SmartPanel::ContactRequestItem_Grid_PointerEntered(Platform::Object ^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ e)
{
    return;
}

void
SmartPanel::ContactRequestItem_Grid_PointerExited(Platform::Object ^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ e)
{
    return;
}

void
SmartPanel::ContactRequestItem_Grid_PointerMoved(Platform::Object ^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs ^ e)
{
    return;
}


void
SmartPanel::_acceptContactRequestBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (Utils::hasInternet()) {
        auto button = dynamic_cast<Button^>(e->OriginalSource);
        if (button) {
            auto item = dynamic_cast<ContactRequestItem^>(button->DataContext);
            if (item) {
                auto contact = item->_contact;
                auto accountId = contact->_accountIdAssociated;
                if (auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountId))) {
                    DRing::acceptTrustRequest(Utils::toString(accountId), Utils::toString(contact->ringID_));
                    contact->_trustStatus = TrustStatus::TRUSTED;
                    contactListModel->saveContactsToFile();

                    auto spi = SmartPanelItemsViewModel::instance->findItem(contact);
                    SmartPanelItemsViewModel::instance->moveItemToTheTop(spi);

                    ContactRequestItemsViewModel::instance->removeItem(item);

                    updateNotificationsState();

                    MSG_("Accepted Contact Request from: " + contact->ringID_);
                }
            }
        }
    }
}

void
SmartPanel::_rejectContactRequestBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (Utils::hasInternet()) {
        auto button = dynamic_cast<Button^>(e->OriginalSource);
        if (button) {
            auto item = dynamic_cast<ContactRequestItem^>(button->DataContext);
            if (item) {
                auto contact = item->_contact;
                auto accountId = contact->_accountIdAssociated;
                if (auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountId))) {
                    DRing::discardTrustRequest(Utils::toString(accountId), Utils::toString(contact->ringID_));
                    contactListModel->deleteContact(contact);
                    contactListModel->saveContactsToFile();

                    ContactRequestItemsViewModel::instance->removeItem(item);

                    updateNotificationsState();

                    MSG_("Discarded Contact Request from: " + contact->ringID_);
                }
            }
        }
    }
}

void
SmartPanel::_blockContactBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    if (Utils::hasInternet()) {
        auto button = dynamic_cast<Button^>(e->OriginalSource);
        if (button) {
            auto item = dynamic_cast<ContactRequestItem^>(button->DataContext);
            if (item) {
                auto contact = item->_contact;
                auto accountId = contact->_accountIdAssociated;
                if (auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountId))) {
                    DRing::removeContact(Utils::toString(accountId), Utils::toString(contact->ringID_), true);

                    contact->_trustStatus = TrustStatus::BLOCKED;
                    contactListModel->saveContactsToFile();

                    ContactRequestItemsViewModel::instance->removeItem(item);

                    updateNotificationsState();

                    MSG_("Blocked Contact with ringId: " + contact->ringID_);
                }
            }
        }
    }
}

void
SmartPanel::OnnewBuddyNotification(const std::string& accountId, const std::string& address, int status)
{
    for (auto account : AccountsViewModel::instance->accountsList) {
        auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(account->accountID_));
        for (auto contact : contactListModel->_contactsList)
            if (contact->ringID_ == Utils::toPlatformString(address))
                contact->_presenceStatus = status;
    }
}

Object^
ContactAccountTypeToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto contact = static_cast<Contact^>(value);
    auto parameterString = static_cast<String^>(parameter);
    auto associatedAccount = AccountsViewModel::instance->findItem(contact->_accountIdAssociated);
    if (associatedAccount->accountType_ == parameterString)
        return VIS::Visible;
    return VIS::Collapsed;
}

Object^
ContactConferenceableToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    // Hide the option until more or less fully functional
    return VIS::Collapsed;

    auto contact = static_cast<Contact^>(value);
    if (SmartPanelItemsViewModel::instance->isInCall()) {
        auto selectedItem = SmartPanelItemsViewModel::instance->_selectedItem;
        if (contact != selectedItem->_contact) {
            return VIS::Visible;
        }
    }
    return VIS::Collapsed;
}

Object^
CachedImageConverter::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto path = static_cast<String^>(value);
    if (path == nullptr)
        return nullptr;
    return RingClientUWP::ResourceMananger::instance->imageFromRelativePath(path);
}

Object^
AccountRegistrationStateToString::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto account = static_cast<Account^>(value);
    auto registrationState = account->_registrationState;

    if (account->accountType_ == "SIP")
        return "";

    if (registrationState == RegistrationState::REGISTERED) {
        return "Online";
    }
    else if (registrationState == RegistrationState::TRYING) {
        return "Offline";
    }
    else if (registrationState == RegistrationState::UNREGISTERED) {
        return "Offline";
    }
    return "Offline";
}

Object^
AccountRegistrationStateToForeground::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto account = static_cast<Account^>(value);
    auto registrationState = account->_registrationState;

    if (registrationState == RegistrationState::REGISTERED) {
        return ref new SolidColorBrush(Utils::ColorFromString("#ff00a000"));
    }
    else if (registrationState == RegistrationState::TRYING) {
        return ref new SolidColorBrush(Utils::ColorFromString("#ffff0000"));
    }
    else if (registrationState == RegistrationState::UNREGISTERED) {
        return ref new SolidColorBrush(Utils::ColorFromString("#ffff0000"));
    }
    return ref new SolidColorBrush(Utils::ColorFromString("#ffff0000"));
}

// This converter will primarily be used to determine the visibility of the
// message bubble spikes.
Object^
MessageChainBreakToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto parameterString = static_cast<String^>(parameter);
    auto messageId = static_cast<std::uint64_t>(value);

    if (SmartPanelItemsViewModel::instance->_selectedItem) {
        auto conversation = SmartPanelItemsViewModel::instance->_selectedItem->_contact->_conversation;
        auto messageIndex = conversation->getMessageIndex(messageId);
        auto messageList = conversation->_messages;

        // If this is the first or last message in the list, then make it visible.
        if (messageIndex == 0 && parameterString != "Last")
            return VIS::Visible;

        if (messageIndex == messageList->Size - 1 && parameterString != "First")
            return VIS::Visible;

        if (parameterString == "First") {
            // The converter is being used to determine if this message is the first
            // of a series. If it is the first message of the chain, return visible,
            // otherwise, return collapsed.
            if (messageList->Size > messageIndex ) {
                if ((!messageList->GetAt(messageIndex)->FromContact &&
                    messageList->GetAt(messageIndex - 1)->FromContact) ||
                    (messageList->GetAt(messageIndex)->FromContact &&
                        !messageList->GetAt(messageIndex - 1)->FromContact)) {
                    return VIS::Visible;
                }
            }
        }
        else if (parameterString == "Last") {
            // The converter is being used to determine if this message is the last
            // of a series. If it is the last message of the chain, return visible,
            // otherwise, return collapsed.
            if (messageList->Size > messageIndex + 1) {
                if ((messageList->GetAt(messageIndex)->FromContact &&
                    !messageList->GetAt(messageIndex + 1)->FromContact) ||
                    (!messageList->GetAt(messageIndex)->FromContact &&
                        messageList->GetAt(messageIndex + 1)->FromContact)) {
                    return VIS::Visible;
                }
            }
        }
    }

    return VIS::Collapsed;
}

Object^
MessageChainBreakToHeight::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto messageId = static_cast<std::uint64_t>(value);

    if (SmartPanelItemsViewModel::instance->_selectedItem) {
        auto conversation = SmartPanelItemsViewModel::instance->_selectedItem->_contact->_conversation;
        auto messageIndex = conversation->getMessageIndex(messageId);
        auto messageList = conversation->_messages;

        // If this is the last message in the list, then make it visible by returning
        // a positive GridLength.
        if (messageIndex == messageList->Size - 1)
            return GridLength(14.0);

        // This converter is being used to determine if this message is the last
        // of a series. If it is the last message of the chain, return a positive
        // GridLength otherwise, return a GridLength of 0.
        if (messageList->Size > messageIndex + 1) {
            if ((messageList->GetAt(messageIndex)->FromContact &&
                !messageList->GetAt(messageIndex + 1)->FromContact) ||
                (!messageList->GetAt(messageIndex)->FromContact &&
                    messageList->GetAt(messageIndex + 1)->FromContact)) {
                return GridLength(14.0);
            }
        }
    }

    return GridLength(0.0);
}

Object^
MessageDateTimeString::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto messageTimestamp = static_cast<std::time_t>(value);
    auto messageDateTime = Utils::epochToDateTime(messageTimestamp);
    auto currentDateTime = Utils::currentDateTime();
    auto currentDay = Utils::dateTimeToString(currentDateTime, "shortdate");
    auto messageDay = Utils::dateTimeToString(messageDateTime, "shortdate");
    if (messageDay != currentDay)
        return Utils::dateTimeToString(messageDateTime, "dayofweek");
    else if (Utils::currentTimestamp() - messageTimestamp > 60)
        return Utils::dateTimeToString(messageDateTime, "hour minute");
    return "Now";
}

Object^
PresenceStatus::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto parameterString = static_cast<String^>(parameter);
    auto presenceStatus = static_cast<int>(value);

    auto offlineColor = ref new SolidColorBrush(Utils::ColorFromString("#00000000"));
    SolidColorBrush^ onlineColor;

    if (parameterString == "Border")
        onlineColor = ref new SolidColorBrush(Utils::ColorFromString("#ffffffff"));
    else
        onlineColor = ref new SolidColorBrush(Utils::ColorFromString("#ff00ff00"));

    return presenceStatus <= 0 ? offlineColor : onlineColor;
}

void
SmartPanel::_smartList__GotFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto frame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Window::Current->Content);
    dynamic_cast<RingClientUWP::MainPage^>(frame->Content)->focusOnMessagingTextbox();
}

void
SmartPanel::_showBannedList__PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    if (_bannedContactListGrid_->Visibility == VIS::Collapsed) {
        _bannedContactListGrid_->Visibility = VIS::Visible;
        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _accountsShowBannedList_->Content = loader->GetString("_accountsHideBannedList_");
    }
    else {
        _bannedContactListGrid_->Visibility = VIS::Collapsed;
        auto loader = ref new Windows::ApplicationModel::Resources::ResourceLoader();
        _accountsShowBannedList_->Content = loader->GetString("_accountsShowBannedList_");
    }
}


void RingClientUWP::Views::SmartPanel::_addBannedContactBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    if (button) {
        auto item = dynamic_cast<SmartPanelItem^>(button->DataContext);
        if (item) {
            auto contact = item->_contact;
            auto accountId = contact->_accountIdAssociated;
            if (auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountId))) {
                DRing::addContact(Utils::toString(accountId), Utils::toString(contact->ringID_));

                contact->_trustStatus = TrustStatus::TRUSTED;
                contactListModel->saveContactsToFile();

                updateNotificationsState();

                MSG_("Added blocked contact to contacts with ringId: " + contact->ringID_);
            }
        }
    }
}


void
SmartPanel::SmartPanelItem_Grid_RightTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::RightTappedRoutedEventArgs^ e)
{
    auto grid = static_cast<Grid^>(sender);
    auto item = static_cast<SmartPanelItem^>(grid->DataContext);
    if (_smartList_->SelectedIndex == -1) {
        // if no item is already selected, select the one right tapped
        _smartList_->SelectedIndex = SmartPanelItemsViewModel::instance->getFilteredIndex(item->_contact);
        item->_isSelected = true;
    }
    auto flyoutOwner = static_cast<FrameworkElement^>(sender);
    auto menuFlyout = static_cast<MenuFlyout^>(FlyoutBase::GetAttachedFlyout(flyoutOwner));
    auto videoCallButton = menuFlyout->Items->GetAt(0);
    videoCallButton->Visibility = item->_isCallable ? VIS::Visible : VIS::Collapsed;
    FlyoutBase::ShowAttachedFlyout(flyoutOwner);
}


void RingClientUWP::Views::SmartPanel::_videocall_MenuFlyoutItem_Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    auto item = static_cast<SmartPanelItem^>(static_cast<MenuFlyoutItem^>(sender)->DataContext);
    placeCall(item);
}

void RingClientUWP::Views::SmartPanel::SmartPanelItem_Grid_DoubleTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::DoubleTappedRoutedEventArgs^ e)
{
    auto grid = static_cast<Grid^>(sender);
    auto item = static_cast<SmartPanelItem^>(grid->DataContext);
    placeCall(item);
}

void RingClientUWP::Views::SmartPanel::_addToConference_MenuFlyoutItem__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    auto item = static_cast<SmartPanelItem^>(static_cast<MenuFlyoutItem^>(sender)->DataContext);
    auto selectedAccount = AccountListItemsViewModel::instance->_selectedItem->_account;
    auto selectedAccountId = selectedAccount->accountID_;
    std::vector<std::string> participantList;
    String^ prefix = selectedAccount->accountType_ == "RING" ? "ring:" : "";
    auto selectedItem = SmartPanelItemsViewModel::instance->_selectedItem;
    participantList.emplace_back(Utils::toString(prefix + selectedItem->_contact->ringID_ + "," + selectedAccountId));
    participantList.emplace_back(Utils::toString(prefix + item->_contact->ringID_ + "," + selectedAccountId));
    DRing::createConfFromParticipantList(participantList);
}


void RingClientUWP::Views::SmartPanel::_copyRingID_MenuFlyoutItem__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    auto item = static_cast<SmartPanelItem^>(static_cast<MenuFlyoutItem^>(sender)->DataContext);
    DataPackage^ dataPackage = ref new DataPackage();
    dataPackage->SetText(item->_contact->ringID_);
    Clipboard::SetContent(dataPackage);
}


void
SmartPanel::_revokeDeviceButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = static_cast<Button^>(sender);
    auto deviceItem = static_cast<RingDeviceItem^>(button->DataContext);

    auto stackPanel = ref new StackPanel();
    stackPanel->Orientation = Orientation::Vertical;
    auto textBlock = ref new TextBlock();
    textBlock->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Center;
    textBlock->FontSize = 14;
    textBlock->Text = "Please enter your account password";
    auto passwordBox = ref new PasswordBox();
    passwordBox->PlaceholderText = "Password";
    passwordBox->Height = 32;
    passwordBox->Margin = Windows::UI::Xaml::Thickness(0.0, 20.0, 0.0, 0.0);
    stackPanel->Children->Append(textBlock);
    stackPanel->Children->Append(passwordBox);
    auto dialog = ref new ContentDialog();
    dialog->Content = stackPanel;
    dialog->Title = "";
    dialog->IsSecondaryButtonEnabled = true;
    dialog->PrimaryButtonText = "Ok";
    dialog->SecondaryButtonText = "Cancel";

    create_task(dialog->ShowAsync())
        .then([=](task<ContentDialogResult> dialogTask) {
        try {
            auto result = dialogTask.get();
            if (result == ContentDialogResult::Primary) {
                auto accountId = Utils::toString(AccountListItemsViewModel::instance->getSelectedAccountId());
                auto password = Utils::toString(passwordBox->Password);
                auto deviceId = Utils::toString(deviceItem->_deviceId);
                RingD::instance->revokeDevice(accountId, password, deviceId);
            }
            else {
                MSG_("revoking device cancelled");
            }
        }
        catch (Exception^ e) {
            EXC_(e);
        }
    });
}


void RingClientUWP::Views::SmartPanel::_deviceName__KeyUp(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::Enter) {
        auto selectedAccount = AccountListItemsViewModel::instance->_selectedItem->_account;
        if (_deviceName_->Text == "") {
            _deviceName_->Text = selectedAccount->_deviceName;
        }
        else {
            MSG_("changing device name, (" + selectedAccount->_deviceName + " -> " + _deviceName_->Text + ")");
            selectedAccount->_deviceName = _deviceName_->Text;
            RingD::instance->updateAccount(selectedAccount->accountID_);
        }
        _deviceName_->IsEnabled = false;
        _deviceName_->IsEnabled = true;
    }
}


void RingClientUWP::Views::SmartPanel::_deviceName__LostFocus(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto selectedAccount = AccountListItemsViewModel::instance->_selectedItem->_account;
    _deviceName_->Text = selectedAccount->_deviceName;
}


void RingClientUWP::Views::SmartPanel::_turnEnabledToggle__Toggled(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto toggleSwitch = dynamic_cast<ToggleSwitch^>(sender);
    auto isTurnEnabledToggleOn = toggleSwitch->IsOn;
    // grey out turn server edit box
}
