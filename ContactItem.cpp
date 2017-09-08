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

#include "ContactItem.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;
using namespace ViewModel;

ContactItem::ContactItem(std::shared_ptr<Models::Contact>& contact)
    :contact_(contact)
{}

ContactItem::ContactItem(Models::Contact& contact)
    : contact_(std::make_shared<Models::Contact>(contact))
{}

ContactItem::ContactItem(Map<String^, String^>^ details)
{
    using namespace Utils;
    auto uri = toString(getDetailsStringValue(details, RingClientUWP::Strings::Contact::URI));
    contact_ = std::make_shared<Models::Contact>(uri);
    SetDetails(details);
}

void
ContactItem::SetDetails(Map<String^, String^>^ details)
{
    using namespace RingClientUWP::Strings::Contact;
    using namespace Utils;

    contact_->displayName       = toString(getDetailsStringValue(details, DISPLAYNAME));
    contact_->registeredName    = toString(getDetailsStringValue(details, REGISTEREDNAME));
    contact_->alias             = toString(getDetailsStringValue(details, ALIAS));
    contact_->isTrusted         = toString(getDetailsStringValue(details, TRUSTED));
    contact_->type              = toString(getDetailsStringValue(details, TYPE));
}



ContactItemList::ContactItemList(String^ accountId)
    : accountId_(accountId)
{
    contactItems_ = ref new Vector<ContactItem^>();
}

ContactItem^
ContactItemList::addItem(Map<String^, String^>^ details)
{
    // Order is not crucial here, as this model only manages an accounts
    // collection of control items, each of which wrap a contact object,
    // and is not responsable for the view at all.
    auto newItem = ref new ContactItem(details);
    contactItems_->Append(newItem);
    return newItem;
}

ContactItem^
ContactItemList::findItem(String^ uri)
{
    for each (ContactItem^ item in contactItems_) {
        if (item->_uri == uri)
            return item;
    }

    return nullptr;
}

ContactItem^
ContactItemList::findItemByAlias(String^ alias)
{
    for each (ContactItem^ item in contactItems_) {
        if (item->_alias == alias)
            return item;
    }

    return nullptr;
}

void
ContactItemList::removeItem(String^ uri)
{
    auto item = findItem(uri);
    unsigned int index;
    contactItems_->IndexOf(item, &index);
    contactItems_->RemoveAt(index);
}
