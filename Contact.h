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
#include "Utils.h"

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
String^ unreadContactRequestKey = "unreadContactRequest";
String^ contactKey = "contact";
String^ contactListKey = "contactlist";
String^ accountIdAssociatedKey = "accountIdAssociated";
String^ vcardUIDKey = "vcardUID";
String^ lastTimeKey = "lastTime";
String^ trustStatusKey = "trustStatus";
String^ isIncognitoContactKey = "isIncognitoContact";
String^ avatarColorStringKey = "avatarColorString";

namespace RingClientUWP
{
ref class Conversation;

public ref class Contact sealed : public INotifyPropertyChanged
{
public:
    Contact(    String^ accountId,
                String^ name,
                String^ ringID,
                String^ GUID,
                uint32 unreadmessages,
                ContactStatus contactStatus,
                TrustStatus trustStatus,
                bool isIncognitoContact,
                String^ avatarColorString);

    JsonObject^ ToJsonObject();

    void raiseNotifyPropertyChanged(String^ propertyName);
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property String^ _name {
        String^ get() {
            return name_;
        }
        void set(String^ value) {
            name_ = value;
            NotifyPropertyChanged("_name");
            NotifyPropertyChanged("_bestName");
            NotifyPropertyChanged("_bestName2");
            NotifyPropertyChanged("_bestName3");
            NotifyPropertyChanged("_avatarColorString");
        }
    }

    property String^ ringID_;
    property String^ GUID_;
    property bool subscribed_;

    property Conversation^ _conversation {
        Conversation^ get() {
            return conversation_;
        }
    }

    property Visibility notificationNewMessage {
        Visibility get() {
            return notificationNewMessage_;
        }
        void set(Visibility visibility) {
            notificationNewMessage_ = visibility;
            NotifyPropertyChanged("notificationNewMessage");
        }
    }

    property uint32 _unreadMessages {
        uint32 get() {
            return unreadMessages_;
        }
        void set(uint32 value) {
            unreadMessages_ = value;
            NotifyPropertyChanged("_unreadMessages");
        }
    }

    property bool _unreadContactRequest {
        bool get() {
            return unreadContactRequest_;
        }
        void set(bool value) {
            unreadContactRequest_ = value;
            NotifyPropertyChanged("_unreadContactRequest");
        }
    }

    property String^ _avatarImage {
        String^ get() {
            return avatarImage_;
        }
        void set(String^ value) {
            avatarImage_ = value;
            NotifyPropertyChanged("_avatarImage");
        }
    }

    property String^ _avatarColorString {
        String^ get() {
            return avatarColorString_;
        }
        void set(String^ value) {
            avatarColorString_ = value;
            NotifyPropertyChanged("_avatarColorString");
            NotifyPropertyChanged("_avatarColorBrush");
        }
    }

    property SolidColorBrush^ _avatarColorBrush {
        SolidColorBrush^ get() {
            return Utils::solidColorBrushFromString(avatarColorString_);
        }
    }

    property Windows::UI::Xaml::GridLength _contactBarHeight {
        GridLength get() {
            return contactBarHeight_;
        }
        void set(GridLength value) {
            contactBarHeight_ = value;
            NotifyPropertyChanged("_contactBarHeight");
        }
    }
    property String^ _accountIdAssociated {
        String^ get() {
            return accountIdAssociated_;
        }
        void set(String^ value) {
            accountIdAssociated_ = value;
            NotifyPropertyChanged("_accountIdAssociated");
        }
    }
    property String^ _vcardUID;

    property String^ _displayName {
        String^ get() {
            return displayName_;
        }
        void set(String^ value) {
            displayName_ = value;
            NotifyPropertyChanged("_displayName");
            NotifyPropertyChanged("_bestName");
        }
    }

    property int _presenceStatus {
        int get() { return presenceStatus_; }
        void set(int value) {
            presenceStatus_ = value;
            NotifyPropertyChanged("_presenceStatus");
        }
    }

    property ContactStatus _contactStatus {
        ContactStatus get() {
            return contactStatus_;
        }
        void set(ContactStatus value) {
            contactStatus_ = value;
            NotifyPropertyChanged("_contactStatus");
        }
    }

    property String^ _lastTime {
        String^ get() {
            return lastTime_;
        }
        void set(String^ value) {
            lastTime_ = value;
            NotifyPropertyChanged("_lastTime");
        }
    }

    property bool _isTrusted {
        bool get() {
            return  trustStatus_ == TrustStatus::CONTACT_REQUEST_SENT ||
                    trustStatus_ == TrustStatus::TRUSTED;
        }
    }

    property String^ _bestName {
        String^ get() {
            String^ bestName;
            if (displayName_)
                bestName += displayName_ + " - ";
            if (name_)
                bestName += name_;
            else if (ringID_)
                bestName += ringID_;
            return bestName;
        }
    }

    property String^ _bestName2 {
        String^ get() {
            String^ bestName;
            if (displayName_)
                bestName = displayName_;
            else if (name_)
                bestName = name_;
            else if (ringID_)
                bestName = ringID_;
            return bestName;
        }
    }

    property String^ _bestName3 {
        String^ get() {
            String^ bestName;
            if (_bestName2 == displayName_) {
                if (name_)
                    bestName = name_;
                else if (ringID_)
                    bestName = ringID_;
                else
                    bestName = "";
            }
            else if (_bestName2 == name_)
                bestName = "";
            else if (_bestName2 == ringID_)
                bestName = "";
            return bestName;
        }
    }

    property TrustStatus _trustStatus {
        TrustStatus get() {
            return trustStatus_;
        }
        void set(TrustStatus value) {
            trustStatus_ = value;
        }
    }

    property bool _isIncognitoContact;

    VCardUtils::VCard^ getVCard();

internal:
    void        saveConversationToFile();
    String^     StringifyConversation();
    void        DestringifyConversation(String^ data);
    void        deleteConversationFile();
    void        loadConversation();

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    VCardUtils::VCard^ vCard_;
    Conversation^ conversation_;
    Visibility notificationNewMessage_;
    unsigned int unreadMessages_;
    int presenceStatus_;
    bool unreadContactRequest_;
    String^ avatarColorString_;
    String^ avatarImage_;
    String^ displayName_;
    String^ accountIdAssociated_;
    Windows::UI::Xaml::GridLength contactBarHeight_ = 0;
    ContactStatus contactStatus_;
    String^ name_;
    String^ lastTime_;
    TrustStatus trustStatus_;
};
}

