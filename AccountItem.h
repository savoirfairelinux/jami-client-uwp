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
#include "Account.h"
#include "ContactListModel.h"

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

using namespace Models;

namespace RingClientUWP
{

namespace Controls
{

public ref class AccountItem sealed
    : public INotifyPropertyChanged
{
public:
    virtual event PropertyChangedEventHandler^ PropertyChanged;

protected:
    void NotifyPropertyChanged(String^ propertyName);

public:
    property String^ _bestName {
        String^ get() {
            String^ bestName;
            if (_alias)
                bestName = _alias;
            else
                bestName = _bestId;
            return bestName;
        }
    }

    property String^ _bestName2 {
        String^ get() {
            String^ bestName;
            if (_accountType == "RING" && _registeredName)
                bestName = _registeredName;
            return bestName;
        }
    }

    property String^ _bestName3 {
        String^ get() {
            String^ bestName;
            if (_alias)
                bestName += _alias;
            if (_accountType == "RING" && _registeredName)
                bestName += " (" + _registeredName + ")";
            return bestName;
        }
    }

    property String^ _bestId {
        String^ get() {
            return (_registeredName) ? _registeredName : _ringId;
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
            return Utils::toPlatformString(account_->username);
        }
        void set(String^ value) {
            account_->username = Utils::toString(value);
            NotifyPropertyChanged("_username");
            NotifyPropertyChanged("_ringId");
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
            NotifyPropertyChanged("_bestName");
            NotifyPropertyChanged("_bestName3");
        }
    }

    property String^ _accountType {
        String^ get() {
            return Utils::toPlatformString(account_->accountType);
        }
        void set(String^ value) {
            account_->accountType = Utils::toString(value);
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

    property bool _dhtPublicInCalls {
        bool get() {
            return account_->dhtPublicInCalls;
        }
        void set(bool value) {
            account_->dhtPublicInCalls = value;
            NotifyPropertyChanged("_dhtPublicInCalls");
        }
    }

    // SIP specific
    property String^ _sipHostname {
        String^ get() {
            return Utils::toPlatformString(account_->hostname);
        }
        void set(String^ value) {
            account_->hostname = Utils::toString(value);
            NotifyPropertyChanged("_sipHostname");
        }
    }

    property String^ _sipUsername {
        String^ get() {
            return Utils::toPlatformString(account_->username);
        }
        void set(String^ value) {
            account_->username = Utils::toString(value);
            NotifyPropertyChanged("_sipUsername");
        }
    }

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
    property String^ _ringId {
        String^ get() {
            return Utils::toPlatformString(account_->username);
        }
    }

    property RegistrationState _registrationState {
        RegistrationState get() {
            return account_->registrationState;
        }
        void set(RegistrationState value) {
            account_->registrationState = value;
            NotifyPropertyChanged("_registrationState");
            NotifyPropertyChanged("_id");
        }
    }

    property String^ _registeredName {
        String^ get() {
            return Utils::toPlatformString(account_->registeredName);
        }
        void set(String^ value) {
            account_->registeredName = Utils::toString(value);
            NotifyPropertyChanged("_registeredName");
            NotifyPropertyChanged("_bestName2");
            NotifyPropertyChanged("_bestName3");
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

    /* contact management */
    property ContactItemList^ _contactItemList {
        ContactItemList^ get() {
            return contactItemList_;
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
    AccountItem(String^ id, Map<String^, String^>^ details);
    void SetDetails(String^ id, Map<String^, String^>^ details);

private:
    std::unique_ptr<Models::Account>    account_;

    ContactItemList^                    contactItemList_;
    std::vector<std::string>            conversationUids_;

    bool                                isSelected_;
};

}
}

