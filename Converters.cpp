/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: J�ger Nicolas <nicolas.jager@savoirfairelinux.com>              *
* Author: Traczyk Andreas <andreas.traczyk@savoirfairelinux.com>          *
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

#include "Converters.h"

#include "TimeUtils.h"
#include "ResourceManager.h"
#include "AccountItemsViewModel.h"
#include "NetUtils.h"
#include "XamlUtils.h"

using namespace RingClientUWP;
using namespace Converters;
using namespace ViewModel;

Object^
FillFromString::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto str = static_cast<String^>(value);
    auto color = Utils::xaml::ColorFromString(Utils::xaml::getAvatarColorStringFromString(str));
    return ref new SolidColorBrush(color);
}

Object^
BubbleBackground::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto c1 = Utils::xaml::ColorFromString("#ffebefef");
    auto c2 = Utils::xaml::ColorFromString("#ffcfebf5");
    return ((bool)value) ? ref new SolidColorBrush(c1) : ref new SolidColorBrush(c2);
}

Object^
BubbleHorizontalAlignement::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    return ((bool)value) ? Windows::UI::Xaml::HorizontalAlignment::Left : Windows::UI::Xaml::HorizontalAlignment::Right;
}

Object^
IncomingVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto state = static_cast<CallStatus>(value);

    if (state == CallStatus::INCOMING_RINGING)
        return  Windows::UI::Xaml::Visibility::Visible;
    else
        return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
OutGoingVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto state = static_cast<CallStatus>(value);

    if (state == CallStatus::SEARCHING
        || state == CallStatus::OUTGOING_RINGING
        || state == CallStatus::OUTGOING_REQUESTED)
        return  Windows::UI::Xaml::Visibility::Visible;
    else
        return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
HasAnActiveCall::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto state = static_cast<CallStatus>(value);

    if (state == CallStatus::NONE || state == CallStatus::ENDED)
        return Windows::UI::Xaml::Visibility::Collapsed;
    else
        return Windows::UI::Xaml::Visibility::Visible;
}

Object^
NewMessageBubbleNotification::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto unreadMessages = static_cast<uint32>(value);

    if (unreadMessages > 0)
        return Windows::UI::Xaml::Visibility::Visible;

    return Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
NewMessageNotificationToNumber::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto unreadMessages = static_cast<uint32>(value);

    if (unreadMessages > 9)
        return "9+";

    return unreadMessages.ToString();
}

Object^
AccountTypeToSourceImage::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto accountType = dynamic_cast<String^>(value);
    Uri^ uri = (accountType == "RING")
        ? ref new Uri("ms-appx:///Assets/AccountTypeRING.png")
        : ref new Uri("ms-appx:///Assets/AccountTypeSIP.png");

    return ref new BitmapImage(uri);
}

Object^
RingAccountTypeToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    if (dynamic_cast<String^>(value) == "RING")
        return VIS::Visible;
    return VIS::Collapsed;
}

Object^
AccountSelectedToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    if ((bool)value == true)
        return Windows::UI::Xaml::Visibility::Visible;

    return Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
CollapseEmptyString::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto stringValue = dynamic_cast<String^>(value);

    return (stringValue->IsEmpty())
        ? Windows::UI::Xaml::Visibility::Collapsed
        : Windows::UI::Xaml::Visibility::Visible;
}

Object^
ContactStatusNotification::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto contactStatus = static_cast<ContactStatus>(value);

    if (contactStatus == ContactStatus::WAITING_FOR_ACTIVATION)
        return Windows::UI::Xaml::Visibility::Visible;
    else
        return Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
boolToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto direction = static_cast<String^>(parameter);
    Visibility if_true = (direction == "Inverted") ? Visibility::Collapsed : Visibility::Visible;
    Visibility if_false = (direction == "Inverted") ? Visibility::Visible : Visibility::Collapsed;
    if (static_cast<bool>(value))
        return if_true;
    return  if_false;
}

Object^
uintToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    if (static_cast<unsigned>(value))
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
OneToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    if (static_cast<unsigned>(value) == 1)
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
UnreadAccountNotificationsString::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto unreadMessages = AccountItemsViewModel::instance->unreadMessages(static_cast<String^>(value));
    auto unreadContactRequests = AccountItemsViewModel::instance->unreadContactRequests(static_cast<String^>(value));
    String^ notificationString;
    std::string notification_string;
    std::string description;
    if (static_cast<String^>(parameter) == "Summary") {
        notification_string = std::to_string(unreadMessages + unreadContactRequests);
    }
    else {
        if (unreadMessages) {
            description = unreadMessages == 1 ? " Message" : " Messages";
            notification_string.append(std::to_string(unreadMessages) + description);
        }
        if (unreadContactRequests) {
            if (unreadMessages)
                notification_string.append(", ");
            description = unreadContactRequests == 1 ? " Contact request" : " Contact requests";
            notification_string.append(std::to_string(unreadContactRequests) + description);
        }
    }
    return Utils::toPlatformString(notification_string);
}

Object^
MoreThanOneToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    if (static_cast<unsigned>(value) > 1)
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
MoreThanZeroToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto unreadMessages = AccountItemsViewModel::instance->unreadMessages(static_cast<String^>(value));
    auto unreadContactRequests = AccountItemsViewModel::instance->unreadContactRequests(static_cast<String^>(value));

    if (unreadMessages + unreadContactRequests > 0)
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
PartialTrustToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto direction = static_cast<String^>(parameter);
    Visibility if_true = (direction == "Inverted") ? Visibility::Collapsed : Visibility::Visible;
    Visibility if_false = (direction == "Inverted") ? Visibility::Visible : Visibility::Collapsed;
    if (static_cast<TrustStatus>(value) == TrustStatus::CONTACT_REQUEST_SENT)
        return if_true;
    return  if_false;
}

Object^
TrustedToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    if (static_cast<Contact^>(value)->_isTrusted)
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
SelectedAccountToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto contact = static_cast<Contact^>(value);
    auto callStatus = SmartPanelItemsViewModel::instance->findItem(contact)->_callStatus;
    auto isCall = (callStatus != CallStatus::NONE && callStatus != CallStatus::ENDED) ? true : false;

    if (contact->_accountIdAssociated == AccountItemsViewModel::instance->getSelectedAccountId() || isCall)
        return Windows::UI::Xaml::Visibility::Visible;

    return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
CallStatusToSpinnerVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto callStatus = static_cast<CallStatus>(value);

    if (callStatus == CallStatus::INCOMING_RINGING
        || callStatus == CallStatus::OUTGOING_REQUESTED
        || callStatus == CallStatus::OUTGOING_RINGING
        || callStatus == CallStatus::SEARCHING)
        return  Windows::UI::Xaml::Visibility::Visible;
    else
        return  Windows::UI::Xaml::Visibility::Collapsed;
}

Object^
CallStatusForIncomingCallEllipse::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto direction = static_cast<String^>(parameter);
    Visibility if_true = (direction == "Inverted") ? Visibility::Collapsed : Visibility::Visible;
    Visibility if_false = (direction == "Inverted") ? Visibility::Visible : Visibility::Collapsed;

    auto callStatus = static_cast<CallStatus>(value);

    if (callStatus == CallStatus::INCOMING_RINGING) {
        return  if_true;
    }
    else {
        return  if_false;
    }
}

Object^
ContactAccountTypeToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    // Hide the option until functional
    return VIS::Collapsed;
}

Object^
ContactConferenceableToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    // Hide the option until functional
    return VIS::Collapsed;
}

Object^
CachedImageConverter::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto path = static_cast<String^>(value);
    if (path == nullptr)
        return nullptr;
    return RingClientUWP::ResourceMananger::instance->imageFromRelativePath(path);
}

Object^
NameToInitialConverter::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto name = static_cast<String^>(value);
    return Utils::getUpperInitial(name);
}

Object^
HasAvatarToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto avatarImage = static_cast<String^>(value);
    auto parameterString = static_cast<String^>(parameter);

    auto positiveResult = parameterString != "Inverted" ? VIS::Visible : VIS::Collapsed;
    auto negtiveResult = parameterString != "Inverted" ? VIS::Collapsed : VIS::Visible;
    return Utils::isEmpty(avatarImage) ? negtiveResult : positiveResult;
}

Object^
AccountRegistrationStateToString::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto accountItem = AccountItemsViewModel::instance->findItem(static_cast<String^>(value));

    if (accountItem->_accountType == "SIP") {
        return Utils::hasInternet() ? "Ready" : "Offline";
    }
    if (!accountItem->_enabled) {
        return "Disabled";
    }
    if (accountItem->_registrationState == RegistrationState::REGISTERED) {
        return "Online";
    }
    return "Offline";
}

Object^
AccountRegistrationStateToForeground::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto accountItem = AccountItemsViewModel::instance->findItem(static_cast<String^>(value));

    if (!accountItem->_enabled) {
        return ref new SolidColorBrush(Utils::xaml::ColorFromString(ErrorColor));
    }
    if (accountItem->_registrationState == RegistrationState::REGISTERED
        || (accountItem->_accountType == "SIP" && Utils::hasInternet())) {
        return ref new SolidColorBrush(Utils::xaml::ColorFromString(SuccessColor));
    }
    return ref new SolidColorBrush(Utils::xaml::ColorFromString(ErrorColor));
}

// This converter will be used to determine the visibility of the
// message bubble spikes.
Object^
MessageChainBreakToVisibility::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    if (SmartPanelItemsViewModel::instance->_selectedItem) {
        auto msgIndex = static_cast<int>(value);
        auto messages = SmartPanelItemsViewModel::instance->_selectedItem->_contact->_conversation->_messages;

        if (!messages->Size)
            return VIS::Collapsed;

        auto message = messages->GetAt(msgIndex);
        auto parameterString = static_cast<String^>(parameter);

        if (parameterString == "First") {
            // The converter is being used to determine if this message is the first
            // of a series. If it is the first message of the chain, return visible,
            // otherwise, return collapsed.
            bool isFirstofSequence = false;
            if (messages->Size) {
                if (msgIndex == 0) {
                    isFirstofSequence = true;
                }
                else {
                    auto previousMessage = messages->GetAt(msgIndex - 1);
                    if (message->FromContact != previousMessage->FromContact)
                        isFirstofSequence = true;
                }
            }
            if (isFirstofSequence)
                return VIS::Visible;
        }
        else if (parameterString == "Last") {
            // The converter is being used to determine if this message is the last
            // of a series. If it is the last message of the chain, return visible,
            // otherwise, return collapsed.
            bool isLastofSequence = false;
            if (msgIndex < messages->Size) {
                if (msgIndex == messages->Size - 1) {
                    isLastofSequence = true;
                }
                else {
                    auto nextMessage = messages->GetAt(msgIndex + 1);
                    if (message->FromContact != nextMessage->FromContact)
                        isLastofSequence = true;
                }
            }
            if (isLastofSequence)
                return VIS::Visible;
        }
    }

    return VIS::Collapsed;
}

// This converter is being used to determine if this message is the last
// of a series. If it is the last message of the chain, return a positive
// GridLength otherwise, return a GridLength of 0.
Object^
MessageChainBreakToHeight::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    if (SmartPanelItemsViewModel::instance->_selectedItem) {
        auto msgIndex = static_cast<int>(value);
        auto messages = SmartPanelItemsViewModel::instance->_selectedItem->_contact->_conversation->_messages;
        if (!messages->Size)
            return GridLength(0.0);

        auto message = messages->GetAt(msgIndex);

        // If this is the last message in the list, then make it visible by returning
        // a positive GridLength
        bool isLastofSequence = false;
        if (msgIndex < messages->Size) {
            if (msgIndex == messages->Size - 1) {
                isLastofSequence = true;
            }
            else {
                auto nextMessage = messages->GetAt(msgIndex + 1);
                if (message->FromContact != nextMessage->FromContact)
                    isLastofSequence = true;
            }
        }
        if (isLastofSequence)
            return GridLength(14.0);
    }

    return GridLength(0.0);
}

Object^
MessageDateTimeString::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    if (SmartPanelItemsViewModel::instance->_selectedItem) {
        auto msgIndex = static_cast<int>(value);
        auto messages = SmartPanelItemsViewModel::instance->_selectedItem->_contact->_conversation->_messages;
        if (!messages->Size)
            return "Now";

        auto message = messages->GetAt(msgIndex);

        bool isLastofSequence = false;
        if (msgIndex < messages->Size) {
            if (msgIndex == messages->Size - 1) {
                isLastofSequence = true;
            }
            else {
                auto nextMessage = messages->GetAt(msgIndex + 1);
                if (message->FromContact != nextMessage->FromContact)
                    isLastofSequence = true;
            }
        }

        if (message && isLastofSequence) {
            auto messageDateTime = Utils::Time::epochToDateTime(message->TimeReceived);
            auto currentDateTime = Utils::Time::currentDateTime();
            auto currentDay = Utils::Time::dateTimeToString(currentDateTime, "shortdate");
            auto messageDay = Utils::Time::dateTimeToString(messageDateTime, "shortdate");
            if (messageDay != currentDay)
                return Utils::Time::dateTimeToString(messageDateTime, "dayofweek");
            else if (Utils::Time::currentTimestamp() - message->TimeReceived > 60)
                return Utils::Time::dateTimeToString(messageDateTime, "hour minute");
        }
    }
    return "Now";
}

Object^
PresenceStatus::Convert(Object ^ value, TypeName targetType, Object ^ parameter, String ^ language)
{
    auto parameterString = static_cast<String^>(parameter);
    auto presenceStatus = static_cast<int>(value);

    auto offlineColor = ref new SolidColorBrush(Utils::xaml::ColorFromString("#00000000"));
    SolidColorBrush^ onlineColor;

    if (parameterString == "Border") {
        onlineColor = ref new SolidColorBrush(Utils::xaml::ColorFromString("#ffffffff"));
    }
    else
        onlineColor = ref new SolidColorBrush(Utils::xaml::ColorFromString(SuccessColor));

    return presenceStatus <= 0 ? offlineColor : onlineColor;
}