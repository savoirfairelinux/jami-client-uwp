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
#pragma once

#include "Utils.h"
#include "Contact.h"

using namespace Platform;

namespace RingClientUWP
{

namespace Controls
{

public ref class ContactItem sealed
{
public:
    property String^ _uri {
        String^ get() {
            return Utils::toPlatformString(contact_->uri);
        }
    }

    property String^ _displayName {
        String^ get() {
            return Utils::toPlatformString(contact_->displayName);
        }
        void set(String^ value) {
            contact_->displayName = Utils::toString(value);
        }
    }

    property String^ _registeredName {
        String^ get() {
            return Utils::toPlatformString(contact_->registeredName);
        }
        void set(String^ value) {
            contact_->registeredName = Utils::toString(value);
        }
    }

    property String^ _alias {
        String^ get() {
            return Utils::toPlatformString(contact_->alias);
        }
        void set(String^ value) {
            contact_->alias = Utils::toString(value);
        }
    }

    property String^ _isTrusted {
        String^ get() {
            return Utils::toPlatformString(contact_->isTrusted);
        }
        void set(String^ value) {
            contact_->isTrusted = Utils::toString(value);
        }
    }

    property String^ _type {
        String^ get() {
            return Utils::toPlatformString(contact_->type);
        }
        void set(String^ value) {
            contact_->type = Utils::toString(value);
        }
    }

    // from db or elsewhere
    property Utils::profile::VCard^ _vCard;
    property unsigned _unreadMessages;
    property unsigned _unreadContactRequest;
    property int _presenceStatus;
    property String^ _avatarColorString;
    property String^ _avatarImage;
    property String^ _accountIdAssociated;
    property String^ _lastTime;
    // TODO: remove these and use CR object and temporary object
    property ContactStatus _contactStatus;
    property TrustStatus _trustStatus;

internal:
    ContactItem() {};
    ContactItem(Models::Contact& contact);
    ContactItem(std::shared_ptr<Models::Contact>& contact);
    ContactItem(Map<String^, String^>^ details);

    void SetDetails(Map<String^, String^>^ details);

private:
    std::shared_ptr<Models::Contact>    contact_;

};

//////////////////////////////
//
// ContactItemList (LRC: ContactModel)
//
//////////////////////////////

public ref class ContactItemList sealed
{
public:
    ContactItemList(String^ accountId);

internal:
    /* functions */
    ContactItem^    addItem(Map<String^, String^>^ details);
    ContactItem^    findItem(String^ uri);
    ContactItem^    findItemByAlias(String^ alias);
    void            removeItem(String^ uri);

public:
    /* properties */
    property IVector<ContactItem^>^ _contactItems {
        IVector<ContactItem^>^ get() {
            return contactItems_;
        }
    }

private:
    IVector<ContactItem^>^ contactItems_;
    String^ accountId_;

};

}
}
