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

public ref class ContactItem sealed : public INotifyPropertyChanged
{

public:
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property String^ _id {
        String^ get() {
            return Utils::toPlatformString(contact_->id);
        }
        void set(String^ value) {
            contact_->id = Utils::toString(value);
        }
    }

    property String^ _displayName {
        String^ get() {
            return Utils::toPlatformString(contact_->displayName);
        }
        void set(String^ value) {
            contact_->displayName = Utils::toString(value);
            NotifyPropertyChanged("_displayName");
        }
    }

    property String^ _username {
        String^ get() {
            return Utils::toPlatformString(contact_->username);
        }
        void set(String^ value) {
            contact_->username = Utils::toString(value);
            NotifyPropertyChanged("_username");
        }
    }

    property String^ _alias {
        String^ get() {
            return Utils::toPlatformString(contact_->alias);
        }
        void set(String^ value) {
            contact_->alias = Utils::toString(value);
            NotifyPropertyChanged("_alias");
        }
    }

    property String^ _ringId {
        String^ get() {
            return Utils::toPlatformString(contact_->ringId);
        }
        void set(String^ value) {
            contact_->ringId = Utils::toString(value);
            NotifyPropertyChanged("_ringId");
        }
    }

    property String^ _contactType {
        String^ get() {
            return Utils::toPlatformString(contact_->contactType);
        }
        void set(String^ value) {
            contact_->contactType = Utils::toString(value);
            NotifyPropertyChanged("_contactType");
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
    ContactItem(Map<String^, String^>^ details);
    void SetDetails(Map<String^, String^>^ details);

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    std::unique_ptr<Models::Contact>    contact_;
    bool                                isSelected_;
};

}
}

