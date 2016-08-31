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

void
Conversation::addMessage(String^ date, bool fromContact, String^ payload)
{
    ConversationMessage^ message = ref new ConversationMessage();
    message->Date = date;
    message->FromContact = fromContact;
    message->Payload = payload;
    std::string owner((fromContact) ? "from contact" : " from me");
    MSG_("{Conversation::addMessage}");
    MSG_("owner = " + owner);

    /* add message to _messagesList_ */
    messagesList_->Append(message);

    // TODO store message on the disk
}

JsonObject^
ConversationMessage::ToJsonObject()
{
    JsonObject^ messageObject = ref new JsonObject();
    messageObject->SetNamedValue(dateKey, JsonValue::CreateStringValue(Date));
    messageObject->SetNamedValue(fromContactKey, JsonValue::CreateBooleanValue(FromContact));
    messageObject->SetNamedValue(payloadKey, JsonValue::CreateStringValue(Payload));

    JsonObject^ jsonObject = ref new JsonObject();
    jsonObject->SetNamedValue(messageKey, messageObject);

    return jsonObject;
}