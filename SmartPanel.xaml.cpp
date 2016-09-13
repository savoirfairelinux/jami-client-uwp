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
#include <string> // move it
#include "SmartPanel.xaml.h"

using namespace Platform;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;
using namespace RingClientUWP::Views;
using namespace RingClientUWP::ViewModel;
using namespace Windows::Media::Capture;
using namespace Windows::UI::Xaml;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Shapes;
using namespace Windows::UI::Xaml::Media;
using namespace Concurrency;
using namespace Windows::Foundation;

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Storage;
using namespace Windows::UI::Core;

SmartPanel::SmartPanel()
{
    InitializeComponent();

    _accountsList_->ItemsSource = AccountsViewModel::instance->accountsList;

    /* populate the smartlist */
    smartPanelItemsList_ = ref new Vector<SmartPanelItem^>();
    _smartList_->ItemsSource = smartPanelItemsList_;

    /* connect delegates */
    Configuration::UserPreferences::instance->selectIndex += ref new SelectIndex([this](int index) {
        _accountsList_->SelectedIndex = index;
    });
    Configuration::UserPreferences::instance->loadProfileImage += ref new LoadProfileImage([this]() {
        StorageFolder^ localfolder = ApplicationData::Current->LocalFolder;
        String^ image_path = localfolder->Path + "\\.profile\\profile_image.png";
        auto uri = ref new Windows::Foundation::Uri(image_path);
        _selectedAccountAvatar_->ImageSource = ref new BitmapImage(uri);
    });
    AccountsViewModel::instance->updateScrollView += ref new UpdateScrollView([this]() {
        _accountsListScrollView_->UpdateLayout();
        _accountsListScrollView_->ScrollToVerticalOffset(_accountsListScrollView_->ScrollableHeight);
    });
    CallsViewModel::instance->callRecieved += ref new RingClientUWP::CallRecieved([&](
    Call^ call) {
        auto from = call->from;
        auto contact = ContactsViewModel::instance->findContactByName(from);

        if (contact == nullptr)
            contact = ContactsViewModel::instance->addNewContact(from, from); // contact checked inside addNewContact.

        if (contact == nullptr) {
            ERR_("contact not handled!");
            return;
        }

        auto item = findItem(contact);
        item->_call = call;
    });
    RingD::instance->stateChange += ref new StateChange([this](String^ callId, String^ state, int code) {
        auto call = CallsViewModel::instance->findCall(callId);

        if (call == nullptr)
            return;

        auto item = findItem(call);

        if (!item) {
            WNG_("item not found");
            return;
        }

        if (call->state == "incoming call")
            item->_IncomingCallBar = Windows::UI::Xaml::Visibility::Visible;

        if (call->state == "CURRENT") {
            item->_IncomingCallBar = Windows::UI::Xaml::Visibility::Collapsed;
            item->_OutGoingCallBar = Windows::UI::Xaml::Visibility::Collapsed;
        }

        if (call->state == "") {
            item->_IncomingCallBar = Windows::UI::Xaml::Visibility::Collapsed;
            item->_OutGoingCallBar = Windows::UI::Xaml::Visibility::Collapsed;
        }
    });


    ContactsViewModel::instance->contactAdded += ref new ContactAdded([this](Contact^ contact) {
        auto smartPanelItem = ref new SmartPanelItem();
        smartPanelItem->_contact = contact;
        smartPanelItemsList_->Append(smartPanelItem);
    });

    RingD::instance->calling += ref new RingClientUWP::Calling([&](
    Call^ call) {
        auto from = call->from;
        auto contact = ContactsViewModel::instance->findContactByName(from);

        if (contact == nullptr) {
            WNG_("cannot call the peer, contact not found!");
            return;
        }

        auto item = findItem(contact);

        if (item == nullptr) {
            WNG_("cannot call the peer, smart panel item not found!");
            return;
        }

        /* use underscore to differentiate states from UI, we need to think more about states management */
        call->state = "_calling_";

        item->_OutGoingCallBar = Windows::UI::Xaml::Visibility::Visible;
        item->_call = call;
    });

}

void
RingClientUWP::Views::SmartPanel::updatePageContent()
{
    auto account = AccountsViewModel::instance->selectedAccount;
    if (!account)
        return;

    Configuration::UserPreferences::instance->PREF_ACCOUNT_INDEX = _accountsList_->SelectedIndex;
    Configuration::UserPreferences::instance->save();
    _selectedAccountName_->Text = account->name_;
}

void RingClientUWP::Views::SmartPanel::_accountsMenuButton__Checked(Object^ sender, RoutedEventArgs^ e)
{
    _shareMenuButton_->IsChecked = false;
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void RingClientUWP::Views::SmartPanel::_accountsMenuButton__Unchecked(Object^ sender, RoutedEventArgs^ e)
{
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void RingClientUWP::Views::SmartPanel::_settings__Checked(Object^ sender, RoutedEventArgs^ e)
{
    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _settings_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void RingClientUWP::Views::SmartPanel::_settings__Unchecked(Object^ sender, RoutedEventArgs^ e)
{
    _settings_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _smartGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void RingClientUWP::Views::SmartPanel::setMode(RingClientUWP::Views::SmartPanel::Mode mode)
{
    if (mode == RingClientUWP::Views::SmartPanel::Mode::Normal) {
        _rowRingTxtBx_->Height = 40;
        _selectedAccountAvatarContainer_->Height = 80;
        _selectedAccountAvatarColumn_->Width = 90;
        _selectedAccountRow_->Height = 90;
    }
    else {
        _rowRingTxtBx_->Height = 0;
        _selectedAccountAvatarContainer_->Height = 50;
        _selectedAccountAvatarColumn_->Width = 60;
        _selectedAccountRow_->Height = 60;
    }

    _selectedAccountAvatarContainer_->Width = _selectedAccountAvatarContainer_->Height;
    _settingsTBtn_->IsChecked = false;
    _accountsMenuButton_->IsChecked = false;
    _shareMenuButton_->IsChecked = false;
}

void RingClientUWP::Views::SmartPanel::_shareMenuButton__Checked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _shareMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _accountsMenuButton_->IsChecked = false;
}

void RingClientUWP::Views::SmartPanel::_shareMenuButton__Unchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _shareMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void RingClientUWP::Views::SmartPanel::_addAccountBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _accountsMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}

void RingClientUWP::Views::SmartPanel::_createAccountYes__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    switch (_accountTypeComboBox_->SelectedIndex)
    {
    case 0:
    {
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
        ref new DispatchedHandler([=]() {
            auto frame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Window::Current->Content);
            dynamic_cast<RingClientUWP::MainPage^>(frame->Content)->showLoadingOverlay(true, true);
        }));
        RingD::instance->createRINGAccount(_aliasTextBox_->Text);
        _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _accountsMenuButton__Checked(nullptr, nullptr);
        break;
    }
    break;
    case 1:
    {
        RingD::instance->createSIPAccount(_aliasTextBox_->Text);
        _accountCreationMenuGrid_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _accountsMenuButton__Checked(nullptr, nullptr);
        break;
    }
    default:
        break;
    }
    _aliasTextBox_->Text = "";
}


void RingClientUWP::Views::SmartPanel::_createAccountNo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _accountsMenuButton_->IsChecked = false;
    _accountsMenuButton__Unchecked(nullptr,nullptr);
}

void
SmartPanel::_smartList__SelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
    auto listbox = safe_cast<ListBox^>(sender);
    auto item = safe_cast<SmartPanelItem^>(listbox->SelectedItem);

    Contact^ contact = (item) ? safe_cast<Contact^>(item->_contact) : nullptr;

    ContactsViewModel::instance->selectedContact = contact;
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
    auto account = safe_cast<Account^>(listbox->SelectedItem);
    AccountsViewModel::instance->selectedAccount = account;
    updatePageContent();
}

void RingClientUWP::Views::SmartPanel::_ringTxtBx__KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    /* add contact, test purpose but will be reused later in some way */
    if (e->Key == Windows::System::VirtualKey::Enter && _ringTxtBx_->Text != "") {
        ContactsViewModel::instance->addNewContact(_ringTxtBx_->Text, _ringTxtBx_->Text);
        _ringTxtBx_->Text = "";
    }
}

// REFACTO : change the name IncomingCall if used with OutGoingCall too.
void RingClientUWP::Views::SmartPanel::_rejectIncomingCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    auto call = dynamic_cast<Call^>(button->DataContext);

    call->refuse();
}


void RingClientUWP::Views::SmartPanel::_acceptIncomingCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    auto call = dynamic_cast<Call^>(button->DataContext);

    call->accept();
}

SmartPanelItem^
SmartPanel::findItem(Contact^ contact)
{
    for each (SmartPanelItem^ item in smartPanelItemsList_)
        if (item->_contact == contact)
            return item;

    return nullptr;
}


SmartPanelItem^
SmartPanel::findItem(Call^ call)
{
    for each (SmartPanelItem^ item in smartPanelItemsList_)
        if (item->_call == call)
            return item;

    return nullptr;
}

void
SmartPanel::_callContact__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    auto item = dynamic_cast<SmartPanelItem^>(button->DataContext);
    auto contact = item->_contact;

    RingD::instance->placeCall(contact);
}


void RingClientUWP::Views::SmartPanel::_cancelCallBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto button = dynamic_cast<Button^>(e->OriginalSource);
    auto call = dynamic_cast<Call^>(button->DataContext);

    call->cancel();
}


void RingClientUWP::Views::SmartPanel::Grid_PointerEntered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto grid = dynamic_cast<Grid^>(sender);
    auto listBoxItem = dynamic_cast<ListBoxItem^>(sender);
    auto item = dynamic_cast<SmartPanelItem^>(grid->DataContext);

    item->_callBar = Windows::UI::Xaml::Visibility::Visible;
}


void RingClientUWP::Views::SmartPanel::Grid_PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto listBoxItem = dynamic_cast<ListBoxItem^>(sender);
    auto grid = dynamic_cast<Grid^>(sender);
    auto item = dynamic_cast<SmartPanelItem^>(grid->DataContext);

    item->_callBar = Windows::UI::Xaml::Visibility::Collapsed;
}


void RingClientUWP::Views::SmartPanel::_contactItem__PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    auto listBoxItem = dynamic_cast<ListBoxItem^>(sender);
    auto smartPanelItem = dynamic_cast<SmartPanelItem^>(listBoxItem->DataContext);

    if (_smartList_->SelectedItem != smartPanelItem)
        _smartList_->SelectedItem = smartPanelItem;
    else
        _smartList_->SelectedItem = nullptr;

}