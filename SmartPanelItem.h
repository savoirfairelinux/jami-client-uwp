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

    virtual event PropertyChangedEventHandler^ PropertyChanged;
    property Contact^ _contact;
    property Visibility _IncomingCallBar
    {
        Visibility get()
        {
            return incomingCallBar_;
        }
        void set(Visibility value)
        {
            incomingCallBar_ = value;
            PropertyChanged(this, ref new PropertyChangedEventArgs("_IncomingCallBar"));
        }
    }
    property Visibility _OutGoingCallBar
    {
        Visibility get()
        {
            return outGoingCallBar_;
        }
        void set(Visibility value)
        {
            outGoingCallBar_ = value;
            PropertyChanged(this, ref new PropertyChangedEventArgs("_OutGoingCallBar"));
        }
    }
    property Visibility _callBar
    {
        Visibility get()
        {
            return callBar_;
        }
        void set(Visibility value)
        {
            callBar_ = value;
            PropertyChanged(this, ref new PropertyChangedEventArgs("_callBar"));
        }
    }
    property Call^ _call
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
    }
    property Visibility _hovered
    {
        Visibility get()
        {
            return hovered_;
        }
        void set(Visibility value)
        {
            hovered_ = value;
            PropertyChanged(this, ref new PropertyChangedEventArgs("_hovered"));
        }
    }
    property uint32 _unreadMessages
    {
        uint32 get()
        {
            return unreadMessages_;
        }
        void set(uint32 value)
        {
            unreadMessages_ = value;
            PropertyChanged(this, ref new PropertyChangedEventArgs("_SPI_unreadMessages"));
        }
    }

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    Visibility incomingCallBar_ = Visibility::Collapsed;
    Visibility outGoingCallBar_ = Visibility::Collapsed;
    Visibility callBar_ = Visibility::Collapsed;
    Call^ call_;
    Visibility hovered_ = Visibility::Collapsed;
    uint32 unreadMessages_;

};
}
}

