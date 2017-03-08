#pragma once
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
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

namespace RingClientUWP
{
namespace Controls
{

public ref class SmartPanelItem sealed : public INotifyPropertyChanged
{
public:
    SmartPanelItem();
    void muteVideo(bool state);

    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property Contact^ _contact;

    property String^ _callId; /*{
        String^ get() {
            return callId_;
        }
        void set(String^ value) {
            _callId = value;
        }
    }*/
    property CallStatus _callStatus {
        CallStatus get() {
            return callStatus_;
        }
        void set(CallStatus value) {
            callStatus_ = value;
            NotifyPropertyChanged("_callStatus");
        }
    }
    property bool _videoMuted // refacto : add set and remove void muteVideo(bool state);
    {
        bool get()
        {
            return videoMuted_;
        }
    }

    property bool _audioMuted;

    property Visibility _showMe
    {
        Visibility get()
        {
            return showMe_;
        }
        void set(Visibility value)
        {
            showMe_ = value;
            NotifyPropertyChanged("_showMe");
        }
    }

    property bool _isSelected
    {
        bool get()
        {
            return isSelected_;
        }
        void set(bool value)
        {
            isSelected_ = value;
            NotifyPropertyChanged("_isSelected");
        }
    }

    property bool _isHovered
    {
        bool get()
        {
            return isHovered_;
        }
        void set(bool value)
        {
            isHovered_ = value;
            NotifyPropertyChanged("_isHovered");
            NotifyPropertyChanged("_isCallable");
        }
    }

    property bool _isCallable
    {
        bool get()
        {
            return ((callStatus_ == CallStatus::ENDED || callStatus_ == CallStatus::NONE) && isHovered_)? true : false;
        }
    }

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    Visibility showMe_ = Visibility::Visible;
    CallStatus callStatus_;
    String^ callId_;
    bool videoMuted_;
    bool isSelected_;
    bool isHovered_;

    void OncallPlaced(Platform::String ^callId);
};
}
}

