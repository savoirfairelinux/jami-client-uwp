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
    RingD::instance->stateChange += ref new RingClientUWP::StateChange(this, &RingClientUWP::ViewModel::SmartPanelItemsViewModel::OnstateChange);
}

bool
SmartPanelItemsViewModel::isInCall()
{
    bool isInCall = false;
    for (auto item : itemsList) {
        if (item->_callId && item->_callStatus == CallStatus::IN_PROGRESS) {
            isInCall = true;
            break;
        }
    }
    return isInCall;
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

void RingClientUWP::ViewModel::SmartPanelItemsViewModel::removeItem(SmartPanelItem ^ item)
{
    unsigned int index;

    if (itemsList->IndexOf(item, &index))
        itemsList->RemoveAt(index);
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
    case CallStatus::ENDED:
    {
        item->_contact->_lastTime = "Last call : " + timestampFormatter->Format(dateTime) + ".";
        String^ accountIdAssociated = getAssociatedAccountId(item);
        if (auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(accountIdAssociated)))
            contactListModel->saveContactsToFile();
        break;
    }
    case CallStatus::IN_PROGRESS:
    {
        item->_contact->_lastTime = "in progress.";
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
SmartPanelItemsViewModel::update()
{
    for each (SmartPanelItem^ item in itemsList) {
        item->notifyPropertyChanged("");
        item->_contact->notifyPropertyChanged("");
    }
}