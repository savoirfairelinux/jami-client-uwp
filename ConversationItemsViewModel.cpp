/**************************************************************************
* Copyright (C) 2017 by Savoir-faire Linux                                *
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

#include "ConversationItemsViewModel.h"

#include "RingD.h"
#include "RingDebug.h"
#include "AccountItemsViewModel.h"
#include "ConversationItem.h"

#include "configurationmanager_interface.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;
using namespace Windows::Globalization::DateTimeFormatting;


using namespace RingClientUWP;
using namespace RingClientUWP::ViewModel;
using namespace RingClientUWP::Controls;

ConversationItemsViewModel::ConversationItemsViewModel()
{
    itemsList_ = ref new Vector<ConversationItem^>();
    itemsListFiltered_ = ref new Vector<ConversationItem^>();
    itemsListBannedFiltered_ = ref new Vector<ConversationItem^>();
}

ConversationItem^
ConversationItemsViewModel::addItem(Object^ item)
{
    ConversationItem^ newConversationItem = nullptr;
    if (auto newContactItem = dynamic_cast<ContactItem^>(item)) {
        newConversationItem = ref new ConversationItem(newContactItem);
    }
    if (newConversationItem) {
        newConversationItem->_avatarImage = ref new String(L" ");
        itemsList_->InsertAt(0, newConversationItem);
    }
    return newConversationItem;
}

ConversationItem^
ConversationItemsViewModel::findItemByCallId(String ^ callId)
{
    for each (ConversationItem^ item in itemsList_) {
        if (item->_callId == callId)
            return item;
    }
    return nullptr;
}

ConversationItem^
ConversationItemsViewModel::findItemByUri(String^ uri)
{
    for each (ConversationItem^ item in itemsList_) {
        if (item->_uri == uri)
            return item;
    }
    return nullptr;
}

unsigned
ConversationItemsViewModel::getIndex(ConversationItem^ item)
{
    int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i) == item)
            break;
    }
    return i;
}

unsigned
ConversationItemsViewModel::getIndexByCallId(String^ callId)
{
    int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_callId == callId)
            break;
    }
    return i;
}

unsigned
ConversationItemsViewModel::getIndexByUri(String^ uri)
{
    int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_uri == uri)
            break;
    }
    return i;
}

void
ConversationItemsViewModel::removeItem(ConversationItem^ item)
{
    unsigned int index;
    itemsList_->IndexOf(item, &index);
    itemsList_->RemoveAt(index);
}

void
ConversationItemsViewModel::moveItemToTheTop(ConversationItem^ item)
{
    unsigned int spi_index, cl_index;

    if (itemsList_->IndexOf(item, &spi_index)) {
        if (spi_index != 0) {
            auto selectedAccountId = getAssociatedAccountId(item);
            if (auto contactItemListModel = AccountItemsViewModel::instance->getContactItemList(selectedAccountId)) {
                auto contactList = contactItemListModel->_contactItems;
                auto contactListItem = contactItemListModel->findItemByAlias(item->_alias);
                contactList->IndexOf(contactListItem, &cl_index);
                contactList->RemoveAt(cl_index);
                contactList->Append(contactListItem);

                itemsList_->RemoveAt(spi_index);
                itemsList_->InsertAt(0, item);
            }
        }
        if (itemsListFiltered_->IndexOf(item, &spi_index)) {
            if (spi_index != 0) {
                itemsListFiltered_->RemoveAt(spi_index);
                itemsListFiltered_->InsertAt(0, item);
                item->_isHovered = false;
            }
        }
    }
}

/*
* Returns true if a call is in progress (across accounts/contacts)
*/
bool
ConversationItemsViewModel::isInCall()
{
    // Check if any items have callIds and are "IN_PROGRESS"
    for (auto item : itemsList_) {
        if (item->_callId && item->_callStatus == CallStatus::IN_PROGRESS) {
            return true;
        }
    }
    return false;
}

String^
ConversationItemsViewModel::getAssociatedAccountId(ConversationItem^ item)
{
    return AccountItemsViewModel::instance->getSelectedAccountId();
}

void
ConversationItemsViewModel::refreshFilteredItemsList()
{
    auto selectedAccountId = AccountItemsViewModel::instance->getSelectedAccountId();
    auto selectedAccountItem = AccountItemsViewModel::instance->_selectedItem;
    auto selectedAccountContactItemList = AccountItemsViewModel::instance->getContactItemList(selectedAccountId);

    std::for_each(begin(itemsList_), end(itemsList_),
        [this, selectedAccountId, selectedAccountItem, selectedAccountContactItemList](ConversationItem^ item) {
            auto isAccountContact = (selectedAccountContactItemList->findItem(item->_uri) != nullptr);

            static unsigned spi_index;
            auto isInList = _itemsListFiltered->IndexOf(item, &spi_index);
            auto isCall = (item->_callStatus != CallStatus::NONE &&
                item->_callStatus != CallStatus::ENDED) ? true : false;
            // 1. Filters out non-account contacts that are NOT incoming calls
            // 2. ***TODO: remove***: Filters out account contacts that don't trust us yet
            if ((isAccountContact || isCall) &&
                (item->_isTrusted ||
                (selectedAccountItem->_dhtPublicInCalls))) {
                if (!isInList) {
                    if (isCall)
                        itemsListFiltered_->InsertAt(0, item);
                    else
                        itemsListFiltered_->Append(item);
                }
            }
            else if (isInList) {
                itemsListFiltered_->RemoveAt(spi_index);
            }
        });

    refreshFilteredBannedItemsList();
}

void
ConversationItemsViewModel::refreshFilteredBannedItemsList()
{
    auto selectedAccountId = AccountItemsViewModel::instance->getSelectedAccountId();
    auto selectedAccountContactItemList = AccountItemsViewModel::instance->getContactItemList(selectedAccountId);

    std::for_each(begin(itemsList_), end(itemsList_),
        [this, selectedAccountId, selectedAccountContactItemList](ConversationItem^ item) {
            auto isAccountContact = (selectedAccountContactItemList->findItem(item->_uri) != nullptr);
            static unsigned spi_index;
            auto isInList = _itemsListBannedFiltered->IndexOf(item, &spi_index);
            // Filters out non-account contacts that are NOT banned
            if (isAccountContact && item->_isBanned) {
                if (!isInList) {
                    _itemsListBannedFiltered->InsertAt(0, item);
                }
            }
            else if (isInList) {
                _itemsListBannedFiltered->RemoveAt(spi_index);
            }
        });
}