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

#include "ThreadUtils.h"
#include "RingDebug.h"
#include "ContactListModel.h"

#include "account_const.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;

AccountItem::AccountItem(String^ id, Map<String^, String^>^ details)
{
    account_ = std::make_unique<Models::Account>();
    contactItemList_ = ref new ContactItemList(id);
    SetDetails(id, details);
}

void
AccountItem::SetDetails(String^ id, Map<String^, String^>^ details)
{
    using namespace DRing::Account;

    _id                 = id;
    _accountType        = Utils::getDetailsStringValue( details, ConfProperties::TYPE);
    _username           = Utils::getDetailsStringValue( details, ConfProperties::USERNAME);
    _hostname           = Utils::getDetailsStringValue( details, ConfProperties::HOSTNAME);
    _alias              = Utils::getDetailsStringValue( details, ConfProperties::ALIAS);
    _autoAnswerEnabled  = Utils::getDetailsBoolValue(   details, ConfProperties::AUTOANSWER);
    _enabled            = Utils::getDetailsBoolValue(   details, ConfProperties::ENABLED);
    _upnpEnabled        = Utils::getDetailsBoolValue(   details, ConfProperties::UPNP_ENABLED);
    _turnEnabled        = Utils::getDetailsBoolValue(   details, ConfProperties::TURN::ENABLED);
    _turnAddress        = Utils::getDetailsStringValue( details, ConfProperties::TURN::SERVER);
    _dhtPublicInCalls   = Utils::getDetailsBoolValue(   details, ConfProperties::DHT::PUBLIC_IN_CALLS);
    _sipPassword        = Utils::getDetailsStringValue( details, ConfProperties::PASSWORD);
    _deviceId           = Utils::getDetailsStringValue( details, ConfProperties::RING_DEVICE_ID);
    _deviceName         = Utils::getDetailsStringValue( details, ConfProperties::RING_DEVICE_NAME);

    auto username = Utils::toString(_username);
    if (username.empty()) {
        ERR_("username empty while parsing account!");
    }
    else if (_accountType == Utils::toPlatformString(ProtocolNames::RING)){
        auto uriPrefixLength = _accountType->Length() + 1;
        _username = Utils::toPlatformString(username.substr(uriPrefixLength));
    }
}

void
AccountItem::NotifyPropertyChanged(String^ propertyName)
{
    Utils::Threading::runOnUIThread([this, propertyName]() {
        PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));
    });
}

