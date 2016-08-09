/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
#include "ContactsViewModel.h"

using namespace RingClientUWP;
using namespace ViewModel;

ContactsViewModel::ContactsViewModel()
{
    contactsList_ = ref new Vector<Contact^>();

    contactsList_->Append(ref new Contact("Homer Simpson", "356373d4fh3d2032d2961f4cbd4e1b46"));
    contactsList_->Append(ref new Contact("Marge Simpson", "b430222a5219a4cb119607f1cdae900e"));
    contactsList_->Append(ref new Contact("Marilyn Manson", "9f9a25b6925b1244f863966f4e33798f"));
    contactsList_->Append(ref new Contact("Jesus Christ", "d1da438329d38517d85d5a523b82ffa8"));
    contactsList_->Append(ref new Contact("Vladimir Lenin", "e38943ae33c7c9cbd8c6512476927ba7"));
    contactsList_->Append(ref new Contact("(de)-crypt master", "45527ef8d4d7b0ba2c3b66342ea0279a"));
    contactsList_->Append(ref new Contact("some people",  "784fe73c815b58233ba020e7ee766911"));
    contactsList_->Append(ref new Contact("some people with a very very very very long name",  "356373d4f63d2032d2961f4cbd4e1b46"));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
    contactsList_->Append(ref new Contact("some people",  ""));
}

Contact^
RingClientUWP::ViewModel::ContactsViewModel::findContactByName(String ^ name)
{
    for each (Contact^ contact in contactsList_)
        if (contact->name_ == name)
            return contact;

    return nullptr;
}

Contact^
RingClientUWP::ViewModel::ContactsViewModel::addNewContact(String^ name, String^ ringId)
{
    if (contactsList_ && !findContactByName(name)) {
        Contact^ contact = ref new Contact(name, ringId);
        contactsList_->Append(contact);
        return contact;
    }

    return nullptr;
}
