/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Traczyk Andreas <traczyk.andreas@savoirfairelinux.com>          *
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

#include "ContactRequestItemsViewModel.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;
using namespace Windows::Globalization::DateTimeFormatting;


using namespace RingClientUWP;
using namespace ViewModel;

ContactRequestItemsViewModel::ContactRequestItemsViewModel()
{
    itemsList_ = ref new Vector<ContactRequestItem^>();
    RingD::instance->stateChange += ref new RingClientUWP::StateChange(this, &RingClientUWP::ViewModel::ContactRequestItemsViewModel::OnstateChange);
}

ContactRequestItem^
ContactRequestItemsViewModel::findItem(Contact^ contact)
{
    for each (ContactRequestItem^ item in itemsList)
        if (item->_contact == contact)
            return item;

    return nullptr;
}

unsigned int
ContactRequestItemsViewModel::getIndex(Contact^ contact)
{
    unsigned int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_contact == contact)
            break;
    }
    return i;
}

void RingClientUWP::ViewModel::ContactRequestItemsViewModel::removeItem(ContactRequestItem ^ item)
{
    unsigned int index;

    if (itemsList->IndexOf(item, &index))
        itemsList->RemoveAt(index);
}

void
ContactRequestItemsViewModel::moveItemToTheTop(ContactRequestItem ^ item)
{
    unsigned int spi_index, cl_index;

    if (itemsList->IndexOf(item, &spi_index)) {
        if (spi_index != 0) {

            auto contactList = ContactsViewModel::instance->contactsList;
            auto contactListItem = ContactsViewModel::instance->findContactByName(item->_contact->_name);
            contactList->IndexOf(contactListItem, &cl_index);
            contactList->RemoveAt(cl_index);
            contactList->Append(contactListItem);
            ContactsViewModel::instance->saveContactsToFile();

            itemsList->RemoveAt(spi_index);
            itemsList->InsertAt(0, item);
            item->_isHovered = false;
        }
    }
}

void
ContactRequestItemsViewModel::OnstateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code)
{

}