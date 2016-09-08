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
    //_smartList_->ItemsSource = ContactsViewModel::instance->contactsList;


    /* populate the smartlist */
    smartPanelItemsList_ = ref new Vector<SmartPanelItem^>();
    _smartList_->ItemsSource = smartPanelItemsList_;
    // has no effect at this stage ecause there is no contact yet in the contacts list
    /* auto contactsList = ContactsViewModel::instance->contactsList;
     for each (auto contact in contactsList) {
         auto smartPanelItem = ref new SmartPanelItem();
         smartPanelItem->_contact = contact;
         smartPanelItemsList_->Append(ref new SmartPanelItem());
     }*/

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

        //bool isNotSelected = (contact != ContactsViewModel::instance->selectedContact) ? true : false;

        if (contact == nullptr) {
            ERR_("contact not handled!");
            return;
        }
        MSG_("#1");
        auto item = findItem(contact);
        item->_call = call;
        MSG_("callid from call object" + Utils::toString(call->callId) + " from item = " + Utils::toString(item->_call->callId));

        //contact->_call = call;
        //contact->_contactBarHeight = 50;

    });
    RingD::instance->stateChange += ref new StateChange([this](String^ callId, String^ state, int code) {
        auto call = CallsViewModel::instance->findCall(callId);

        MSG_("@1");
        if (call == nullptr)
            return;

        MSG_("@2");
        auto item = findItem(call);

        if (!item)
            MSG_("item not found");
        else
            MSG_(" state : " + Utils::toString(item->_call->state));

        if (call->state == "incoming call") {
            MSG_("@3");
            item->_callBar = Windows::UI::Xaml::Visibility::Visible;
        }

        if (call->state == "CURRENT") {
            MSG_("@5");
            item->_callBar = Windows::UI::Xaml::Visibility::Collapsed;
        }


        if (call->state == "") {
            MSG_("@4");
            item->_callBar = Windows::UI::Xaml::Visibility::Collapsed;
        }

    });


    ContactsViewModel::instance->contactAdded += ref new ContactAdded([this](Contact^ contact) {
        auto smartPanelItem = ref new SmartPanelItem();
        smartPanelItem->_contact = contact;
        smartPanelItemsList_->Append(smartPanelItem);
        MSG_("contact : "+Utils::toString(contact->name_)+", size = "+std::to_string(smartPanelItemsList_->Size));
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
    auto contact = safe_cast<Contact^>(item->_contact);
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