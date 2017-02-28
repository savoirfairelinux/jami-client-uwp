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
#pragma once

using namespace Platform;
using namespace Windows::UI::Xaml::Data;
using namespace Platform::Collections;
using namespace Windows::Foundation::Collections;

namespace RingClientUWP
{
ref class Contact;

public ref class Account sealed : public INotifyPropertyChanged
{
public:
    Account(String^ name,
            String^ ringID,
            String^ accountType,
            String^ accountID,
            String^ deviceId,
            bool upnpState,
            String^ sipHostname,
            String^ sipUsername,
            String^ sipPassword);

    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property String^ name_
    {
        String^ get() {
            return alias_;
        }
        void set(String^ value) {
            alias_ = value;
            NotifyPropertyChanged("name_");
        }
    }
    property String^ ringID_ {
        String^ get() {
            return ringID__;
        }
        void set(String^ value) {
            ringID__ = value;
            NotifyPropertyChanged("ringID_");
        }
    }
    property String^ accountType_; // refacto : create a enum accountType
    property String^ accountID_;
    property String^ _deviceId;
    property IVector<String^>^ _devicesIdList {
        IVector<String^>^ get() {
            return devicesIdList_;
        }
        void set(IVector<String^>^ value) {
            devicesIdList_ = value;
        }
    };
    property bool _upnpState;
    property String^ _sipHostname;
    property String^ _sipUsername
    {
        String^ get() {
            return sipUsername_;
        }
        void set(String^ value) {
            sipUsername_ = value;
            NotifyPropertyChanged("_sipUsername");
        }
    }

    property unsigned _unreadMessages
    {
        unsigned get() {
            return unreadMessages_;
        }
        void set(unsigned value) {
            unreadMessages_ = value;
            NotifyPropertyChanged("_unreadMessages");
        }
    }

    property unsigned _unreadContactRequests
    {
        unsigned get() {
            return unreadContactRequests_;
        }
        void set(unsigned value) {
            unreadContactRequests_ = value;
            NotifyPropertyChanged("_unreadContactRequests");
        }
    }

    property String^ _sipPassword; // refacto : think to encrypt password in memory

    property IVector<Contact^>^ _contactsList {
        IVector<Contact^>^ get() {
            return contactsList_;
        }
        void set(IVector<Contact^>^ value) {
            contactsList_ = value;
        }
    };

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    IVector<String^>^ devicesIdList_;
    IVector<Contact^>^ contactsList_;

    String^ alias_;
    String^ ringID__;
    String^ sipUsername_;
    unsigned unreadMessages_;
    unsigned unreadContactRequests_;
};
}

