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
    contact_ = std::make_shared<Models::Contact>();
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
