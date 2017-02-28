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

#include "SmartPanelItemsViewModel.h"

#include "configurationmanager_interface.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;
using namespace Windows::Globalization::DateTimeFormatting;


using namespace RingClientUWP;
using namespace ViewModel;

SmartPanelItemsViewModel::SmartPanelItemsViewModel()
{
    itemsList_ = ref new Vector<SmartPanelItem^>();
    itemsListFiltered_ = ref new Vector<SmartPanelItem^>();
    itemsListBannedFiltered_ = ref new Vector<SmartPanelItem^>();

    RingD::instance->stateChange += ref new RingClientUWP::StateChange(this, &RingClientUWP::ViewModel::SmartPanelItemsViewModel::OnstateChange);
}

bool
SmartPanelItemsViewModel::isInCall()
{
    for (auto item : itemsList) {
        if (item->_callId && item->_callStatus == CallStatus::IN_PROGRESS)
            return true;
    }
    return false;
}

SmartPanelItem^
SmartPanelItemsViewModel::findItem(String^ callId)
{
    for each (SmartPanelItem^ item in itemsList)
        if (item->_callId == callId)
            return item;

    return nullptr;
}

SmartPanelItem^
SmartPanelItemsViewModel::findItem(Contact^ contact)
{
    for each (SmartPanelItem^ item in itemsList)
        if (item->_contact == contact)
            return item;

    return nullptr;
}

SmartPanelItem^
SmartPanelItemsViewModel::findItemByRingID(String^ ringID)
{
    for each (SmartPanelItem^ item in itemsList)
        if (item->_contact->ringID_ == ringID)
            return item;

    return nullptr;
}

unsigned int
SmartPanelItemsViewModel::getIndex(String^ callId)
{
    unsigned int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_callId == callId)
            break;
    }
    return i;
}

unsigned int
SmartPanelItemsViewModel::getIndex(Contact^ contact)
{
    unsigned int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_contact == contact)
            break;
    }
    return i;
}

unsigned int
SmartPanelItemsViewModel::getFilteredIndex(Contact^ contact)
{
    unsigned int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsListFiltered_->GetAt(i)->_contact == contact)
            break;
    }
    return i;
}

void RingClientUWP::ViewModel::SmartPanelItemsViewModel::removeItem(SmartPanelItem ^ item)
{
    unsigned int index;

    if (itemsList->IndexOf(item, &index)) {
        itemsList->RemoveAt(index);

        if (itemsListFiltered->IndexOf(item, &index)) {
            itemsListFiltered->RemoveAt(index);
        }
    }
}

void RingClientUWP::ViewModel::SmartPanelItemsViewModel::moveItemToTheTop(SmartPanelItem ^ item)
{
    unsigned int spi_index, cl_index;

    if (itemsList->IndexOf(item, &spi_index)) {
        if (spi_index != 0) {
            String^ accountIdAssociated = getAssociatedAccountId(item);
            if (auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountIdAssociated))) {
                auto contactList = contactListModel->_contactsList;
                auto contactListItem = contactListModel->findContactByName(item->_contact->_name);
                contactList->IndexOf(contactListItem, &cl_index);
                contactList->RemoveAt(cl_index);
                contactList->Append(contactListItem);
                contactListModel->saveContactsToFile();

                itemsList->RemoveAt(spi_index);
                itemsList->InsertAt(0, item);
            }
        }
        if (itemsListFiltered->IndexOf(item, &spi_index)) {
            if (spi_index != 0) {
                itemsListFiltered->RemoveAt(spi_index);
                itemsListFiltered->InsertAt(0, item);
                item->_isHovered = false;
            }
        }
    }
}

void RingClientUWP::ViewModel::SmartPanelItemsViewModel::OnstateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code)
{
    auto item = SmartPanelItemsViewModel::instance->findItem(callId);

    if (!item) {
        WNG_("item not found");
        return;
    }

    item->_callStatus = state;

    Windows::Globalization::Calendar^ calendar = ref new Windows::Globalization::Calendar();
    Windows::Foundation::DateTime dateTime = calendar->GetDateTime();
    auto timestampFormatter = ref new DateTimeFormatter("day month year hour minute second");

    switch (state) {
    case CallStatus::NONE:
    {
        break;
    }
    case CallStatus::ENDED:
    {
        item->_contact->_lastTime = "Last call : " + timestampFormatter->Format(dateTime) + ".";
        String^ accountIdAssociated = getAssociatedAccountId(item);
        if (auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountIdAssociated)))
            contactListModel->saveContactsToFile();
        refreshFilteredItemsList();
        break;
    }
    case CallStatus::IN_PROGRESS:
    {
        item->_contact->_lastTime = "in progress.";
        item->startCallTimer();
        break;
    }
    case CallStatus::PEER_PAUSED:
    {
        item->_contact->_lastTime = "paused by "+ item->_contact->_name+".";
        break;
    }
    case CallStatus::PAUSED:
    {
        item->_contact->_lastTime = "paused.";
        break;
    }
    case CallStatus::OUTGOING_REQUESTED:
    case CallStatus::OUTGOING_RINGING:
    case CallStatus::SEARCHING:
    {
        item->_contact->_lastTime = "looking for " + item->_contact->_name + ".";
        break;
    }
    case CallStatus::INCOMING_RINGING:
        item->_contact->_lastTime = "incoming call from " + item->_contact->_name + ".";
        refreshFilteredItemsList();
        break;
    case CallStatus::AUTO_ANSWERING:
        item->_contact->_lastTime = "in progress.";
        refreshFilteredItemsList();
        break;
    default:
        break;
    }
}

String^
SmartPanelItemsViewModel::getAssociatedAccountId(SmartPanelItem^ item)
{
    if (item->_contact->_accountIdAssociated->IsEmpty())
        return AccountListItemsViewModel::instance->_selectedItem->_account->accountID_;
    else
        return item->_contact->_accountIdAssociated;
}

void
SmartPanelItemsViewModel::update(const std::vector<std::string>& properties)
{
    for each (SmartPanelItem^ item in itemsListFiltered_) {
        for each (std::string prop in properties) {
            item->raiseNotifyPropertyChanged(Utils::toPlatformString(prop));
            item->_contact->raiseNotifyPropertyChanged(Utils::toPlatformString(prop));
        }
    }
}

void
SmartPanelItemsViewModel::refreshFilteredItemsList()
{
    auto selectedAccountId = AccountListItemsViewModel::instance->getSelectedAccountId();

    std::for_each(begin(itemsList_), end(itemsList_),
        [selectedAccountId, this](SmartPanelItem^ item) {
        static unsigned spi_index;
        auto isInList = itemsListFiltered->IndexOf(item, &spi_index);
        auto isCall = ( item->_callStatus != CallStatus::NONE &&
                        item->_callStatus != CallStatus::ENDED ) ? true : false;
        auto account = AccountsViewModel::instance->findItem(item->_contact->_accountIdAssociated);
        // 1. Filters out non-account contacts that are NOT incoming calls
        // 2. Filters out account contacts that don't trust us yet
        if ((item->_contact->_accountIdAssociated == selectedAccountId || isCall) &&
            (item->_contact->_isTrusted ||
            (account->_dhtPublicInCalls && item->_contact->_isIncognitoContact &&
             item->_contact->_trustStatus == TrustStatus::INCOMING_CONTACT_REQUEST ||
             item->_contact->_trustStatus == TrustStatus::UNKNOWN))) {
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
SmartPanelItemsViewModel::refreshFilteredBannedItemsList()
{
    auto selectedAccountId = AccountListItemsViewModel::instance->getSelectedAccountId();

    std::for_each(begin(itemsList_), end(itemsList_),
        [selectedAccountId, this](SmartPanelItem^ item) {
        static unsigned spi_index;
        auto isInList = itemsListBannedFiltered->IndexOf(item, &spi_index);
        auto isBanned = item->_contact->_trustStatus == TrustStatus::BLOCKED ? true : false;
        auto account = AccountsViewModel::instance->findItem(item->_contact->_accountIdAssociated);
        // Filters out non-account contacts that are NOT banned
        if (item->_contact->_accountIdAssociated == selectedAccountId && isBanned) {
            if (!isInList) {
                itemsListBannedFiltered->InsertAt(0, item);
            }
        }
        else if (isInList) {
            itemsListBannedFiltered->RemoveAt(spi_index);
        }
    });
}