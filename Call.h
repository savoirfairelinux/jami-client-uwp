#pragma once
/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
using namespace Windows::UI::Xaml::Data;

namespace RingClientUWP
{
/* enumerations. */
public enum class CallStatus { NONE, INCOMING_RINGING, OUTGOING_RINGING, SEARCHING, IN_PROGRESS, ENDED };

public ref class CallStatusText sealed : IValueConverter {
public:
    virtual Object^ Convert(Object^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object^ parameter, String^ language);
    virtual Object^ ConvertBack(Object^ value, Windows::UI::Xaml::Interop::TypeName  targetType, Object^ parameter, String^ language);
    CallStatusText();
};

public ref class Call sealed : public INotifyPropertyChanged
{
public:

    /* functions */
    Call(String^ accountId, String^ callId, String^ from);
    void stateChange(CallStatus state, int code);

    /* properties */
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property String^ accountId;
    property String^ callId;
    property String^ from;
    property CallStatus state {
        CallStatus get() {
            return state_;
        }
        void set(CallStatus value) {
            state_ = value;
            PropertyChanged(this, ref new PropertyChangedEventArgs("state"));
        }
    }
    property bool isOutGoing;
    property int code;

    /* events */

protected:
    /* properties */
    void NotifyPropertyChanged(String^ propertyName);

internal:
    void refuse();
    void accept();
    void cancel();

private:
    CallStatus state_;

};
}

