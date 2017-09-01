/***************************************************************************
 * Copyright (C) 2016 by Savoir-faire Linux                                *
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

#include "AccountItemsViewModel.h"

#include "RingD.h"
#include "UserPreferences.h"

#include "account_const.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace ViewModel;

AccountItemsViewModel::AccountItemsViewModel()
{
    itemsList_ = ref new Vector<AccountItem^>();
}

void
AccountItemsViewModel::addItem(String^ id, Map<String^, String^>^ details)
{
    itemsList_->InsertAt(0, ref new AccountItem(id, details));
    Configuration::UserPreferences::instance->raiseSelectIndex(0);
    auto accountType = Utils::getDetailsStringValue(details, DRing::Account::ConfProperties::TYPE);
    if (accountType == Utils::toPlatformString(DRing::Account::ProtocolNames::RING)) {
        auto ringID = Utils::getDetailsStringValue(details, DRing::Account::ConfProperties::USERNAME);
        RingD::instance->lookUpAddress(Utils::toString(id), ringID);
    }
}

AccountItem^
AccountItemsViewModel::findItem(String^ id)
{
    for each (AccountItem^ item in itemsList_) {
        if (item->_id == id)
            return item;
    }

    return nullptr;
}

void
AccountItemsViewModel::removeItem(String ^ id)
{
    auto item = findItem(id);
    unsigned int index;
    itemsList_->IndexOf(item, &index);
    itemsList_->RemoveAt(index);
}

int
AccountItemsViewModel::getIndex(String^ id)
{
    int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_id == id)
            break;
    }
    return i;
}

String^
AccountItemsViewModel::getSelectedAccountId()
{
    if (_selectedItem)
        return _selectedItem->_id;
    return nullptr;
}

AccountItem^
AccountItemsViewModel::findItemByRingID(String ^ ringId)
{
    for each (AccountItem^ item in itemsList_)
        if (item->_ringId == ringId)
            return item;

    return nullptr;
}

void
AccountItemsViewModel::clearAccountList()
{
    itemsList_->Clear();
    accountItemsCleared();
}

ContactItemList^
AccountItemsViewModel::getContactItemList(String^ id)
{
    if (auto item = findItem(id))
        return item->_contactItemList;
    return nullptr;
}

int
AccountItemsViewModel::unreadMessages(String ^ accountId)
{
    int messageCount = 0;
    // count messages
    return messageCount;
}

int
AccountItemsViewModel::unreadContactRequests(String ^ accountId)
{
    int contactRequestCount = 0;
    // count contact requests
    return contactRequestCount;
}

int
AccountItemsViewModel::bannedContacts(String^ accountId)
{
    int bannedContacts = 0;
    // count banned contacts
    return bannedContacts;
}