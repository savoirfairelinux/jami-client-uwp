/***************************************************************************
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

#include "AccountListItemsViewModel.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;


using namespace RingClientUWP;
using namespace ViewModel;

AccountListItemsViewModel::AccountListItemsViewModel()
{
    itemsList_ = ref new Vector<AccountListItem^>();

    /* connect to delegates */
    AccountsViewModel::instance->accountAdded += ref new RingClientUWP::AccountAdded(this, &RingClientUWP::ViewModel::AccountListItemsViewModel::OnaccountAdded);
    AccountsViewModel::instance->clearAccountsList += ref new RingClientUWP::ClearAccountsList(this, &RingClientUWP::ViewModel::AccountListItemsViewModel::OnclearAccountsList);
}

void RingClientUWP::ViewModel::AccountListItemsViewModel::OnaccountAdded(RingClientUWP::Account ^account)
{
    auto item = ref new AccountListItem(account);
    itemsList_->InsertAt(0, item);
}


void RingClientUWP::ViewModel::AccountListItemsViewModel::OnclearAccountsList()
{
    itemsList_->Clear();
}

void
AccountListItemsViewModel::updateContactsViewModel()
{
    SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
    SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);
}

AccountListItem^
RingClientUWP::ViewModel::AccountListItemsViewModel::findItem(String^ accountId)
{
    for each (AccountListItem^ item in itemsList_)
        if (item->_account->accountID_ == accountId)
            return item;

    return nullptr;
}

int
AccountListItemsViewModel::getIndex(String^ accountId)
{
    int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_account->accountID_ == accountId)
            break;
    }
    return i;
}

void RingClientUWP::ViewModel::AccountListItemsViewModel::removeItem(AccountListItem ^ item)
{
    unsigned int index;
    itemsList_->IndexOf(item, &index);

    item->_disconnected = true; // avoid disconected exception.

    itemsList_->RemoveAt(index);

}

String^
AccountListItemsViewModel::getSelectedAccountId()
{
    if (_selectedItem)
        return _selectedItem->_account->accountID_;
    return nullptr;
}

int
AccountListItemsViewModel::unreadMessages()
{
    int messageCount = 0;
    for each (auto account in AccountsViewModel::instance->accountsList) {
        account->_unreadMessages = AccountsViewModel::instance->unreadMessages(account->accountID_);
        messageCount += account->_unreadMessages;
    }
    return messageCount;
}

int
AccountListItemsViewModel::unreadContactRequests()
{
    int unreadContactRequestCount = 0;
    for each (auto account in AccountsViewModel::instance->accountsList) {
        account->_unreadContactRequests = AccountsViewModel::instance->unreadContactRequests(account->accountID_);
        unreadContactRequestCount += account->_unreadContactRequests;
    }
    return unreadContactRequestCount;
}

void
AccountListItemsViewModel::update(const std::vector<std::string>& properties)
{
    for each (AccountListItem^ item in itemsList) {
        item->raiseNotifyPropertyChanged("");
        for each (std::string prop in properties) {
            item->_account->raiseNotifyPropertyChanged(Utils::toPlatformString(prop));
        }
    }
}