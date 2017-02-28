/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
using namespace Windows::Data::Json;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

#include <chrono>

namespace RingClientUWP
{
namespace Controls
{

public ref class SmartPanelItem sealed : public INotifyPropertyChanged
{
public:
    SmartPanelItem();

    void raiseNotifyPropertyChanged(String^ propertyName);
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    void muteVideo(bool state);
    void startCallTimer();

    property Contact^ _contact {
        Contact^ get() { return contact_; }
        void set(Contact^ value) {
            contact_ = value;
            NotifyPropertyChanged("_contact");
        }
    };

    property String^ _callId {
        String^ get() {
            return Utils::toPlatformString(call_.id);
        }
        void set(String^ value) {
            call_.id = Utils::toString(value);
        }
    };

    property String^ _callTime {
        String^ get() {
            auto duration = std::chrono::steady_clock::now() - call_.callStartTime;
            auto hours = std::chrono::duration_cast<std::chrono::hours>(duration).count();
            auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration).count() - hours * 60;
            auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration).count() - minutes * 60;
            auto hoursString = !hours ? "" : (hours < 10 ? "0" + hours.ToString() : hours.ToString()) + ":";
            auto minutesString = minutes < 10 ? "0" + minutes.ToString() : minutes.ToString();
            auto secondsString = seconds < 10 ? "0" + seconds.ToString() : seconds.ToString();
            return hoursString + minutesString + ":" + secondsString;
        }
    };

    property CallStatus _callStatus {
        CallStatus get() { return callStatus_; }
        void set(CallStatus value) {
            callStatus_ = value;
            NotifyPropertyChanged("_callStatus");
        }
    }

    property bool _videoMuted {
        bool get() { return videoMuted_; }
    }

    property bool _audioMuted;

    property Visibility _isVisible {
        Visibility get() { return isVisible_; }
        void set(Visibility value) {
            isVisible_ = value;
            NotifyPropertyChanged("_isVisible");
            NotifyPropertyChanged("_isCallable");
        }
    }

    property bool _isSelected {
        bool get() { return isSelected_; }
        void set(bool value) {
            isSelected_ = value;
            NotifyPropertyChanged("_isSelected");
            NotifyPropertyChanged("_isCallable");
        }
    }

    property bool _isHovered {
        bool get() { return isHovered_; }
        void set(bool value) {
            isHovered_ = value;
            NotifyPropertyChanged("_isHovered");
            NotifyPropertyChanged("_isCallable");
        }
    }

    property bool _isCallable {
        bool get() {
            return ((   callStatus_ == CallStatus::ENDED ||
                        callStatus_ == CallStatus::NONE)
                        && isHovered_) ? true : false;
        }
    }

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    Visibility isVisible_;
    bool isSelected_;
    bool isHovered_;

    CallStatus callStatus_;
    Contact^ contact_;
    String^ callId_;
    Call call_;
    bool videoMuted_;

    void OncallPlaced(Platform::String ^callId);
};
}
}

