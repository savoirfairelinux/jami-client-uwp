/***************************************************************************
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

#include "pch.h"

#include "Account.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::UI::Core;

using namespace RingClientUWP;

Account::Account(String^ name,
                 String^ ringID,
                 String^ accountType,
                 String^ accountID,
                 String^ deviceId,
                 String^ deviceName,
                 bool active,
                 bool upnpState,
                 bool autoAnswer,
                 bool dhtPublicInCalls,
                 bool turnEnabled,
                 String^ turnAddress,
                 String^ sipHostname,
                 String^ sipUsername,
                 String^ sipPassword)
{
    name_ = name;
    ringID_ = ringID;
    accountType_ = accountType;
    accountID_ = accountID;
    _deviceId = deviceId;
    _deviceName = deviceName;
    _active = active;
    _upnpState = upnpState;
    _autoAnswer = autoAnswer;
    _dhtPublicInCalls = dhtPublicInCalls;
    _turnEnabled = turnEnabled;
    _turnAddress = turnAddress;
    _sipHostname = sipHostname;
    _sipUsername = sipUsername;
    _sipPassword = sipPassword;
    _unreadMessages = 0;
    _registrationState = RegistrationState::UNKNOWN;
    _username = "";
}

void
Account::NotifyPropertyChanged(String^ propertyName)
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
Account::raiseNotifyPropertyChanged(String^ propertyName)
{
    NotifyPropertyChanged(propertyName);
}
