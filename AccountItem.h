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
#pragma once

#include "Utils.h"

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

namespace RingClientUWP
{

namespace Controls
{

public ref class AccountItem sealed
    : public INotifyPropertyChanged
    , public SmartItemBase
{

public:
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property String^ _bestName {
        String^ get() {
            String^ bestName;
            if (_alias)
                bestName = _alias;
            return bestName;
        }
    }

    property String^ _bestName2 {
        String^ get() {
            String^ bestName;
            if (_accountType == "RING" && _username)
                bestName = _username;
            return bestName;
        }
    }

    property String^ _bestName3 {
        String^ get() {
            String^ bestName;
            if (_alias)
                bestName += _alias;
            if (_accountType == "RING" && _username)
                bestName += " (" + _username + ")";
            return bestName;
        }
    }

    property String^ _bestId {
        String^ get() {
            return (_username) ? _username : _ringId;
        }
    }

    //
    // DETAILS
    //
    property String^ _id {
        String^ get() {
            return Utils::toPlatformString(account_->id);
        }
        void set(String^ value) {
            account_->id = Utils::toString(value);
        }
    }

    property String^ _username {
        String^ get() {
            return Utils::toPlatformString(account_->hostname);
        }
        void set(String^ value) {
            account_->hostname = Utils::toString(value);
            NotifyPropertyChanged("_username");
        }
    }

    property String^ _hostname {
        String^ get() {
            return Utils::toPlatformString(account_->hostname);
        }
        void set(String^ value) {
            account_->hostname = Utils::toString(value);
            NotifyPropertyChanged("_hostname");
        }
    }

    property String^ _alias {
        String^ get() {
            return Utils::toPlatformString(account_->alias);
        }
        void set(String^ value) {
            account_->alias = Utils::toString(value);
            NotifyPropertyChanged("_alias");
        }
    }

    property String^ _accountType {
        String^ get() {
            return Utils::toPlatformString(account_->accountType);
        }
        void set(String^ value) {
            account_->accountType = Utils::toString(value);
            NotifyPropertyChanged("_accountType");
        }
    }

    property bool _autoAnswerEnabled {
        bool get() {
            return account_->autoAnswerEnabled;
        }
        void set(bool value) {
            account_->autoAnswerEnabled = value;
            NotifyPropertyChanged("_autoAnswerEnabled");
        }
    }

    property bool _enabled {
        bool get() {
            return account_->enabled;
        }
        void set(bool value) {
            account_->enabled = value;
            NotifyPropertyChanged("_enabled");
        }
    }

    // Ring specific
    property bool _upnpEnabled {
        bool get() {
            return account_->upnpEnabled;
        }
        void set(bool value) {
            account_->upnpEnabled = value;
            NotifyPropertyChanged("_upnpEnabled");
        }
    }

    property bool _turnEnabled {
        bool get() {
            return account_->turnEnabled;
        }
        void set(bool value) {
            account_->turnEnabled = value;
            NotifyPropertyChanged("_turnEnabled");
        }
    }

    property String^ _turnAddress {
        String^ get() {
            return Utils::toPlatformString(account_->turnAddress);
        }
        void set(String^ value) {
            account_->turnAddress = Utils::toString(value);
            NotifyPropertyChanged("_turnAddress");
        }
    }

    property bool _publicDhtInCalls {
        bool get() {
            return account_->publicDhtInCalls;
        }
        void set(bool value) {
            account_->publicDhtInCalls = value;
            NotifyPropertyChanged("_publicDhtInCalls");
        }
    }

    // SIP specific
    property String^ _sipPassword {
        String^ get() {
            return Utils::toPlatformString(account_->sipPassword);
        }
        void set(String^ value) {
            account_->sipPassword = Utils::toString(value);
            NotifyPropertyChanged("_sipPassword");
        }
    }
    //
    // END DETAILS
    //

    //
    // Ring specific
    //
    property RegistrationState _registrationState {
        RegistrationState get() {
            return account_->registrationState;
        }
        void set(RegistrationState value) {
            account_->registrationState = value;
            NotifyPropertyChanged("_registrationState");
        }
    }

    property String^ _ringId {
        String^ get() {
            return Utils::toPlatformString(account_->ringId);
        }
        void set(String^ value) {
            account_->ringId = Utils::toString(value);
            NotifyPropertyChanged("_ringId");
        }
    }

    property String^ _deviceId {
        String^ get() {
            return Utils::toPlatformString(account_->deviceId);
        }
        void set(String^ value) {
            account_->deviceId = Utils::toString(value);
            NotifyPropertyChanged("_deviceId");
        }
    }

    property String^ _deviceName {
        String^ get() {
            return Utils::toPlatformString(account_->deviceName);
        }
        void set(String^ value) {
            account_->deviceName = Utils::toString(value);
            NotifyPropertyChanged("_deviceName");
        }
    }

    // selection
    property bool _isSelected {
        bool get() {
            return isSelected_;
        }
        void set(bool value) {
            isSelected_ = value;
            NotifyPropertyChanged("_isSelected");
        }
    }

internal:
    AccountItem(Map<String^, String^>^ details);
    void SetDetails(Map<String^, String^>^ details);

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    std::unique_ptr<Models::Account>    account_;
    bool                                isSelected_;
};

}
}

