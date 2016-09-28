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
#include "pch.h"

#include "Call.h"

using namespace Windows::ApplicationModel::Core;
using namespace Platform;
using namespace Windows::UI::Core;

using namespace RingClientUWP;

// REFACTORING : for the whole Call class, change "from" to "peer"

Call::Call(String^ accountIdz, String^ callIdz, String^ fromz)
{
    this->accountId = accountIdz;
    this->callId = callIdz;
    this->from = fromz;

    isOutGoing = false; // by default, we consider the call incomming, REFACTO : add this to the constructor params...

    this->state = CallStatus::NONE;
    this->code = -1;
}

void RingClientUWP::Call::stateChange(CallStatus state, int code)
{
    this->state = state;
    PropertyChanged(this, ref new PropertyChangedEventArgs("state"));
    this->code = code;
}

void
Call::NotifyPropertyChanged(String^ propertyName)
{
    CoreApplicationView^ view = CoreApplication::MainView;
    view->CoreWindow->Dispatcher->RunAsync(
        CoreDispatcherPriority::Normal,
        ref new DispatchedHandler([this, propertyName]()
    {
        PropertyChanged(this, ref new PropertyChangedEventArgs(propertyName));

    }));
}

void RingClientUWP::Call::refuse()
{
    RingD::instance->refuseIncommingCall(this);
}

void RingClientUWP::Call::accept()
{
    RingD::instance->acceptIncommingCall(this);
}

void RingClientUWP::Call::cancel()
{
    RingD::instance->cancelOutGoingCall(this);
}

Object ^ RingClientUWP::CallStatusText::Convert(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    /*auto callStatus = static_cast<CallStatus>(value);



    switch (callStatus)
    {
    case CallStatus::NONE :
        return ref new String(L"none");
    case CallStatus::ENDED:
        return ref new String(L"ended");
    case CallStatus::INCOMING_RINGING:
    case CallStatus::OUTGOING_RINGING:
        return ref new String(L"ringing");
    case CallStatus::IN_PROGRESS:
        return ref new String(L"in progress");
    case CallStatus::SEARCHING:
        return ref new String(L"searching");
    }*/

    return ref new String(L"undefined");

}

Object ^ RingClientUWP::CallStatusText::ConvertBack(Object ^ value, Windows::UI::Xaml::Interop::TypeName targetType, Object ^ parameter, String ^ language)
{
    throw ref new Platform::NotImplementedException();
}

RingClientUWP::CallStatusText::CallStatusText()
{}
