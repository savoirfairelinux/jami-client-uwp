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
namespace Controls {
public ref class SmartPanelItem sealed : public INotifyPropertyChanged
{
public:
    SmartPanelItem();
    void muteVideo(bool state);

    virtual event PropertyChangedEventHandler^ PropertyChanged;
    property Contact^ _contact;
    /*property Call^ _call
    {
        Call^ get()
        {
            return call_;
        }
        void set(Call^ value)
        {
            call_ = value;
            PropertyChanged(this, ref new PropertyChangedEventArgs("_call"));
        }
    }*/
    property Visibility _hovered
    {
        Visibility get()
        {
            return hovered_;
        }
        void set(Visibility value)
        {
            hovered_ = value;
            NotifyPropertyChanged("_hovered");
        }
    }

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
    property bool _videoMuted
    {
        bool get()
        {
            return videoMuted_;
        }
    }

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

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    Visibility hovered_ = Visibility::Collapsed;
    Visibility showMe_ = Visibility::Visible;
    CallStatus callStatus_;
    String^ callId_;
    bool videoMuted_;

    void OncallPlaced(Platform::String ^callId);
};
}
}

