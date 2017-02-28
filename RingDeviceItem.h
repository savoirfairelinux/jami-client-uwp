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

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

namespace RingClientUWP
{

namespace Controls
{

public ref class RingDeviceItem sealed : public INotifyPropertyChanged
{

public:
    RingDeviceItem(String^ deviceId, String^ deviceName);

    void raiseNotifyPropertyChanged(String^ propertyName);
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property String^ _deviceName {
        String^ get() {
            return Utils::toPlatformString(device_->name);
        }
        void set(String^ value) {
            device_->name = Utils::toString(value);
            NotifyPropertyChanged("_deviceName");
        }
    }

    property String^ _deviceId {
        String^ get() {
            return Utils::toPlatformString(device_->id);
        }
        void set(String^ value) {
            device_->id = Utils::toString(value);
            NotifyPropertyChanged("_deviceId");
        }
    }

    property bool _isSelected {
        bool get() {
            return isSelected_;
        }
        void set(bool value) {
            isSelected_ = value;
            NotifyPropertyChanged("_isSelected");
        }
    }

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    std::unique_ptr<RingDevice>     device_;

    bool                            isSelected_;
};
}
}

