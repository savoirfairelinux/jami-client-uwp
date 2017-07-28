/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
#pragma once

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Documents;

#define NOTIMPLEMENTED                                      \
    {                                                       \
        throw ref new Platform::NotImplementedException();  \
    };                                                      \

namespace RingClientUWP
{

namespace Converters
{

public ref class BubbleBackground sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        BubbleBackground() {};
};

public ref class BubbleHorizontalAlignement sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        BubbleHorizontalAlignement() {};
};

public ref class ContactAccountTypeToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        ContactAccountTypeToVisibility() {};
};

public ref class ContactConferenceableToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        ContactConferenceableToVisibility() {};
};

public ref class CachedImageConverter sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        CachedImageConverter() {};
};

public ref class NameToInitialConverter sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        NameToInitialConverter() {};
};

public ref class HasAvatarToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        HasAvatarToVisibility() {};
};

public ref class AccountRegistrationStateToString sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        AccountRegistrationStateToString() {};
};

public ref class AccountRegistrationStateToForeground sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        AccountRegistrationStateToForeground() {};
};

public ref class MessageChainBreakToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        MessageChainBreakToVisibility() {};
};

public ref class MessageChainBreakToHeight sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        MessageChainBreakToHeight() {};
};

public ref class MessageDateTimeString sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        MessageDateTimeString() {};
};

public ref class IncomingVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        IncomingVisibility() {};
};

public ref class PresenceStatus sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        PresenceStatus() {};
};

public ref class OutGoingVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        OutGoingVisibility() {};
};

public ref class HasAnActiveCall sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        HasAnActiveCall() {};
};

public ref class AccountTypeToSourceImage sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        AccountTypeToSourceImage() {};
};

public ref class RingAccountTypeToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        RingAccountTypeToVisibility() {};
};

public ref class AccountSelectedToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        AccountSelectedToVisibility() {};
};

public ref class NewMessageNotificationToNumber sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        NewMessageNotificationToNumber() {};
};

public ref class NewMessageBubbleNotification sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        NewMessageBubbleNotification() {};
};

public ref class CollapseEmptyString sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        CollapseEmptyString() {};
};

public ref class ContactStatusNotification sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        ContactStatusNotification() {};
};

public ref class boolToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        boolToVisibility() {};
};

public ref class uintToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        uintToVisibility() {};
};

public ref class OneToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        OneToVisibility() {};
};

public ref class UnreadAccountNotificationsString sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        UnreadAccountNotificationsString() {};
};

public ref class MoreThanOneToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        MoreThanOneToVisibility() {};
};

public ref class MoreThanZeroToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        MoreThanZeroToVisibility() {};
};

public ref class PartialTrustToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        PartialTrustToVisibility() {};
};

public ref class TrustedToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        TrustedToVisibility() {};
};

public ref class SelectedAccountToVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        SelectedAccountToVisibility() {};
};

public ref class CallStatusToSpinnerVisibility sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        CallStatusToSpinnerVisibility() {};
};

public ref class CallStatusForIncomingCallEllipse sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, TypeName  targetType, Object^ parameter, String^ language) NOTIMPLEMENTED
        CallStatusForIncomingCallEllipse() {};
};

}/* using namespace Converters */

}/* using namespace RingClientUWP */