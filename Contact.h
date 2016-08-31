#pragma once
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
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

/* strings required by Windows::Data::Json. Defined here on puprose */
String^ nameKey = "name";
String^ ringIDKey = "ringid";
String^ GUIDKey = "guid";
String^ unreadMessagesKey = "unreadmessages";
String^ contactKey = "contact";
String^ contactListKey = "contactlist";

namespace RingClientUWP
{
ref class Conversation;
public ref class Contact sealed : public INotifyPropertyChanged
{
public:
    Contact(String^ name, String^ ringID, String^ GUID, unsigned int unreadmessages);
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
            PropertyChanged(this, ref new PropertyChangedEventArgs("notificationNewMessage"));
        }
    }
    property String^ unreadMessages
    {
        String^ get()
        {
            return unreadMessages_.ToString();
        }
    }

internal:
    void        saveConversationToFile();
    String^     StringifyConversation();
    void        DestringifyConversation(String^ data);
    void        addNotifyNewConversationMessage();

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    Conversation^ conversation_;
    Visibility notificationNewMessage_;
    unsigned int unreadMessages_;

};
}

