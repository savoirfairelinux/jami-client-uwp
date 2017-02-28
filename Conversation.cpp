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
#include "pch.h"

#include "Conversation.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::UI::Core;

using namespace RingClientUWP;

Conversation::Conversation()
{
    messagesList_ = ref new  Vector<ConversationMessage^>();
}

ConversationMessage^
Conversation::findMessage(uint64_t messageId)
{
    for each (ConversationMessage^ message in messagesList_)
        if (message->MessageIdInteger == messageId)
            return message;
    return nullptr;
}

unsigned
Conversation::getMessageIndex(uint64_t messageId)
{
    unsigned i;
    for (i = 0; i < messagesList_->Size; i++) {
        if (messagesList_->GetAt(i)->MessageIdInteger == messageId)
            break;
    }
    return i;
}

void
Conversation::addMessage(bool fromContact,
                         String^ payload,
                         std::time_t timeReceived,
                         bool isReceived,
                         String^ MessageId)
{
    ConversationMessage^ message = ref new ConversationMessage();
    message->FromContact = fromContact;
    message->Payload = payload;
    message->TimeReceived = timeReceived;
    message->IsReceived = isReceived;
    message->MessageId = MessageId;

    /* add message to _messagesList_ */
    messagesList_->Append(message);

    update();
}

void
Conversation::update()
{
    for each (ConversationMessage^ message in messagesList_) {
        message->raiseNotifyPropertyChanged("");
    }
}

JsonObject^
ConversationMessage::ToJsonObject()
{
    JsonObject^ messageObject = ref new JsonObject();
    messageObject->SetNamedValue(fromContactKey, JsonValue::CreateBooleanValue(FromContact));
    messageObject->SetNamedValue(payloadKey, JsonValue::CreateStringValue(Payload));
    messageObject->SetNamedValue(timeReceivedKey, JsonValue::CreateNumberValue(static_cast<double>(TimeReceived)));
    messageObject->SetNamedValue(isReceivedKey, JsonValue::CreateBooleanValue(IsReceived));
    messageObject->SetNamedValue(messageIdKey, JsonValue::CreateStringValue(MessageId));

    JsonObject^ jsonObject = ref new JsonObject();
    jsonObject->SetNamedValue(messageKey, messageObject);

    return jsonObject;
}

void
ConversationMessage::NotifyPropertyChanged(String^ propertyName)
{
    CoreApplicationView^ view = CoreApplication::MainView;
    view->CoreWindow->Dispatcher->RunAsync(
        CoreDispatcherPriority::High,
        ref new DispatchedHandler([this, propertyName]()
    {
        PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
    }));
}

void
ConversationMessage::raiseNotifyPropertyChanged(String^ propertyName)
{
    NotifyPropertyChanged(propertyName);
}

String^
ConversationMessage::getMessageAvatar()
{
    if (ViewModel::SmartPanelItemsViewModel::instance->_selectedItem)
        return ViewModel::SmartPanelItemsViewModel::instance->_selectedItem->_contact->_avatarImage;
    return nullptr;
}