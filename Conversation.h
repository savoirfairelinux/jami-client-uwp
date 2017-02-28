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

using namespace Platform;
using namespace Windows::UI::Xaml::Data;

/* strings required by Windows::Data::Json. Defined here on puprose */
String^ conversationKey = "conversation";
String^ messageKey      = "message";
String^ fromContactKey  = "fromContact";
String^ payloadKey      = "payload";
String^ timeReceivedKey = "timeReceived";
String^ isReceivedKey   = "isReceived";
String^ messageIdKey    = "messageId";

namespace RingClientUWP
{

public ref class ConversationMessage sealed : public INotifyPropertyChanged
{
public:
    void raiseNotifyPropertyChanged(String^ propertyName);
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property bool FromContact;
    property String^ Payload;
    property std::time_t TimeReceived;
    property bool IsReceived;
    property String^ MessageId;
    property int MessageIndex;

    property String^ MessageAvatar {
        String^ get() {
            return getMessageAvatar();
        }
    }

    property SolidColorBrush^ MessageAvatarColorBrush {
        SolidColorBrush^ get() {
            return getMessageAvatarColorBrush();
        }
    }

    property String^ MessageAvatarInitial {
        String^ get() {
            return getMessageAvatarInitial();
        }
    }

    property uint64_t MessageIdInteger {
        uint64_t get() {
            return strtoull(Utils::toString(MessageId).c_str(), nullptr, 0);
        }
    }

    /* functions */
    JsonObject^ ToJsonObject();
    String^ getMessageAvatar();
    SolidColorBrush^ getMessageAvatarColorBrush();
    String^ getMessageAvatarInitial();

protected:
    void NotifyPropertyChanged(String^ propertyName);

};

public ref class Conversation sealed
{
public:
    Conversation();

    /* functions */
    void addMessage(bool fromContact,
                    String^ payload,
                    std::time_t timeReceived,
                    bool isReceived,
                    String^ MessageId);

internal:
    /* properties */
    property IObservableVector<ConversationMessage^>^ _messages {
        IObservableVector<ConversationMessage^>^ get() {
            return messagesList_;
        }
    }

    /* functions */
    void update(const std::vector<std::string>& properties);
    ConversationMessage^ findMessage(uint64_t messageId);
    unsigned getMessageIndex(uint64_t messageId);

private:
    /* members */
    IObservableVector<ConversationMessage^>^ messagesList_;
    ListBox^ messageListBox_;

};
#define MSG_FROM_CONTACT true
#define MSG_FROM_ME false
}

