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

#include "Globals.h"
#include "TimeUtils.h"

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

namespace Models
{
namespace Conversation
{
namespace Message
{
public enum class Type {
    TEXT,
    CALL,
    INVITE,
    INVALID_TYPE
};

public enum class Status {
    SENDING,
    FAILED,
    SUCCEEDED,
    INVALID_STATUS
};

struct Info
{
    Info() { };
    Info(const std::string& uid)
        : uid(uid)
        , body("")
        , from("")
        , timestamp(0)
        , isOutgoing(false)
        , type(Type::INVALID_TYPE)
        , status(Status::INVALID_STATUS)
    { }

    const std::string           uid;
    std::string                 body;
    std::string                 from;
    std::time_t                 timestamp;
    bool                        isOutgoing;
    Type                        type;
    Status                      status;
};

} /*namespace Conversation*/
} /*namespace Conversation*/
} /*namespace Models*/

using MessageType = Models::Conversation::Message::Type;
using MessageStatus = Models::Conversation::Message::Status;
using Message = Models::Conversation::Message::Info;

namespace Controls
{
public ref class NewConversationMessage sealed
    : public INotifyPropertyChanged
{
public:
    virtual event PropertyChangedEventHandler^ PropertyChanged;

protected:
    void NotifyPropertyChanged(String^ propertyName);

public:
    property String^ _uid {
        String^ get() {
            return Utils::toPlatformString(message_->uid);
        }
    }

    property String^ _body {
        String^ get() { return Utils::toPlatformString(message_->body); }
        void set(String^ value) {
            message_->body = Utils::toString(value);
            NotifyPropertyChanged("_body");
        }
    }

    property String^ _from {
        String^ get() { return Utils::toPlatformString(message_->from); }
        void set(String^ value) {
            message_->from = Utils::toString(value);
            NotifyPropertyChanged("_from");
        }
    }

    property DateTime _timestamp {
        DateTime get() { return Utils::Time::epochToDateTime(message_->timestamp); }
        void set(DateTime value) {
            message_->timestamp = Utils::Time::dateTimeToEpoch(value);
            NotifyPropertyChanged("_timestamp");
        }
    }

    property bool _isOutgoing {
        bool get() { return message_->isOutgoing; }
        void set(bool value) {
            message_->isOutgoing = value;
            NotifyPropertyChanged("_body");
        }
    }

    property MessageType _type {
        MessageType get() { return message_->type; }
        void set(MessageType value) {
            message_->type = value;
            NotifyPropertyChanged("_type");
        }
    }

    property MessageStatus _status {
        MessageStatus get() { return message_->status; }
        void set(MessageStatus value) {
            message_->status = value;
            NotifyPropertyChanged("_status");
        }
    }

    /*helpers*/
    property String^ _messageAvatar {
        String^ get() {
            return getMessageAvatar();
        }
    }

    property SolidColorBrush^ _messageAvatarColorBrush {
        SolidColorBrush^ get() {
            return getMessageAvatarColorBrush();
        }
    }

    property String^ _messageAvatarInitial {
        String^ get() {
            return getMessageAvatarInitial();
        }
    }

    property uint64_t _messageIdInteger {
        uint64_t get() {
            return strtoull(message_->uid.c_str(), nullptr, 0);
        }
    }

    /* functions */
    String^             getMessageAvatar();
    SolidColorBrush^    getMessageAvatarColorBrush();
    String^             getMessageAvatarInitial();

internal:
    NewConversationMessage(String^ uid);

private:
    std::unique_ptr<Message> message_;
};
}

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

};
#define MSG_FROM_CONTACT true
#define MSG_FROM_ME false
}

