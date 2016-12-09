/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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

#include "VCardUtils.h"

using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

/* strings required by Windows::Data::Json. Defined here on puprose */
String^ nameKey = "name";
String^ displayNameKey = "displayname";
String^ ringIDKey = "ringid";
String^ GUIDKey = "id";
String^ unreadMessagesKey = "unreadmessages";
String^ contactKey = "contact";
String^ contactListKey = "contactlist";
String^ accountIdAssociatedKey = "accountIdAssociated";
String^ vcardUIDKey = "vcardUID";

namespace RingClientUWP
{
ref class Conversation;

public ref class Contact sealed : public INotifyPropertyChanged
{
public:
    Contact(String^ name, String^ ringID, String^ GUID, unsigned int unreadmessages, ContactStatus contactStatus);
    JsonObject^ ToJsonObject();

    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property String^ name_;
    property String^ ringID_;
    property String^ GUID_;

    property Conversation^ _conversation
    {
        Conversation^ get()
        {
            return conversation_;
        }
    }
    property Visibility notificationNewMessage
    {
        Visibility get()
        {
            return notificationNewMessage_;
        }
        void set(Visibility visibility)
        {
            notificationNewMessage_ = visibility;
            NotifyPropertyChanged("notificationNewMessage");
        }
    }
    property uint32 _unreadMessages
    {
        uint32 get()
        {
            return unreadMessages_;
        }
        void set(uint32 value)
        {
            unreadMessages_ = value;
            NotifyPropertyChanged("_unreadMessages");
        }
    }
    property String^ _avatarImage
    {
        String^ get()
        {
            return avatarImage_;
        }
        void set(String^ value)
        {
            avatarImage_ = value;
            NotifyPropertyChanged("_avatarImage");
        }
    }
    property Windows::UI::Xaml::GridLength _contactBarHeight
    {
        Windows::UI::Xaml::GridLength get()
        {
            return contactBarHeight_;
        }
        void set(Windows::UI::Xaml::GridLength value)
        {
            contactBarHeight_ = value;
            NotifyPropertyChanged("_contactBarHeight");
        }
    }
    property String^ _accountIdAssociated;
    property String^ _vcardUID;
    property String^ _displayName
    {
        String^ get()
        {
            return displayName_;
        }
        void set(String^ value)
        {
            displayName_ = value;
            NotifyPropertyChanged("_displayName");
        }
    }

    property ContactStatus _contactStatus
    {
        ContactStatus get()
        {
            return contactStatus_;
        }
        void set(ContactStatus value)
        {
            contactStatus_ = value;
            NotifyPropertyChanged("_contactStatus");
        }
    }

    VCardUtils::VCard^ getVCard();

internal:
    void        saveConversationToFile();
    String^     StringifyConversation();
    void        DestringifyConversation(String^ data);
    void        deleteConversationFile();


protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    VCardUtils::VCard^ vCard_;
    Conversation^ conversation_;
    Visibility notificationNewMessage_;
    unsigned int unreadMessages_;
    String^ avatarImage_;
    String^ displayName_;
    Windows::UI::Xaml::GridLength contactBarHeight_ = 0;
    ContactStatus contactStatus_;
};
}

