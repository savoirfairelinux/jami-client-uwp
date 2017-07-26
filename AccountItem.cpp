/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
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

#include "AccountItem.h"

#include "account_const.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;

AccountItem::AccountItem(Map<String^, String^>^ details)
{
    account_ = std::make_unique<Models::Account>();
    SetDetails(details);
}

void
AccountItem::SetDetails(Map<String^, String^>^ details)
{
    using namespace DRing::Account::ConfProperties;

    _id                 = Utils::getDetailsStringValue( details, ID);
    _username           = Utils::getDetailsStringValue( details, USERNAME);
    _hostname           = Utils::getDetailsStringValue( details, HOSTNAME);
    _alias              = Utils::getDetailsStringValue( details, ALIAS);
    _accountType        = Utils::getDetailsStringValue( details, TYPE);
    _autoAnswerEnabled  = Utils::getDetailsBoolValue(   details, AUTOANSWER);
    _enabled            = Utils::getDetailsBoolValue(   details, ENABLED);
    _upnpEnabled        = Utils::getDetailsBoolValue(   details, UPNP_ENABLED);
    _turnEnabled        = Utils::getDetailsBoolValue(   details, TURN::ENABLED);
    _turnAddress        = Utils::getDetailsStringValue( details, TURN::SERVER);
    _publicDhtInCalls   = Utils::getDetailsBoolValue(   details, DHT::PUBLIC_IN_CALLS);
    _sipPassword        = Utils::getDetailsStringValue( details, PASSWORD);
    _deviceName         = Utils::getDetailsStringValue( details, RING_DEVICE_NAME);
}

void
AccountItem::NotifyPropertyChanged(String^ propertyName)
{
    Utils::runOnUIThread([this, propertyName]() {
        PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
    });
}
