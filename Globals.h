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

namespace RingClientUWP
{

using namespace Platform;

namespace Strings
{

namespace Account
{

namespace Type
{
constexpr static const char RING[] = "RING";
constexpr static const char SIP[] = "SIP";
constexpr static const char INVALID_TYPE[] = "INVALID_TYPE";
}

}

namespace Contact
{
constexpr static const char URI[] = "Contact.uri";
constexpr static const char DISPLAYNAME[] = "Contact.displayName";
constexpr static const char REGISTEREDNAME[] = "Contact.registeredName";
constexpr static const char ALIAS[] = "Contact.alias";
constexpr static const char TRUSTED[] = "Contact.isTrusted";
constexpr static const char TYPE[] = "Contact.type";

namespace Type
{
constexpr static const char RING[] = "RING";
constexpr static const char SIP[] = "SIP";
constexpr static const char INVALID_TYPE[] = "INVALID_TYPE";
}

namespace Presence
{
constexpr static const char ONLINE[] = "ONLINE";
constexpr static const char OFFLINE[] = "OFFLINE";
constexpr static const char UNKNOWN[] = "UNKNOWN";
}

}

constexpr static const char TRUE_STRING[] = "TRUE";
constexpr static const char FALSE_STRING[] = "FALSE";
}

String^ SuccessColor = "#FF00CC6A";
String^ ErrorColor = "#FFFF4343";

namespace ViewModel
{
namespace NotifyStrings
{
const std::vector<std::string> notifySmartPanelItem = {
    "_isSelected",
    "_contactStatus",
    "_lastTime",
    "_presenceStatus",
    "_displayName",
    "_name",
    "_bestName",
    "_bestName2",
    "notificationNewMessage",
    "_unreadMessages",
    "_unreadContactRequest",
    "_trustStatus" };
const std::vector<std::string> notifyContactRequestItem = {
    "_isSelected" };
const std::vector<std::string> notifyConversation = { "" };
}
}

/* public enumerations. */
public enum class CallStatus {
    NONE,
    OUTGOING_REQUESTED,
    INCOMING_RINGING,
    OUTGOING_RINGING,
    SEARCHING,
    IN_PROGRESS,
    PAUSED,
    PEER_PAUSED,
    ENDED,
    TERMINATING,
    CONNECTED,
    AUTO_ANSWERING
};

public enum class DeviceRevocationResult {
    SUCCESS,
    INVALID_PASSWORD,
    INVALID_CERTIFICATE
};

public enum class TrustStatus {
    UNKNOWN,
    INCOMING_CONTACT_REQUEST,
    INGNORED,
    BLOCKED,
    CONTACT_REQUEST_SENT,
    TRUSTED
};

public enum class RegistrationState {
    UNKNOWN,
    TRYING,
    REGISTERED,
    UNREGISTERED
};

public enum class LookupStatus {
    SUCCESS,
    INVALID_NAME,
    NOT_FOUND,
    ERRORR // one cannot simply use ERROR
};

public enum class ContactStatus {
    WAITING_FOR_ACTIVATION,
    READY
};

/* public enumerations. */
constexpr bool DEBUG_ON = true;

}