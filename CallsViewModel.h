#pragma once
/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: J�ger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
using namespace Platform::Collections;

namespace RingClientUWP
{
/* delegate */
delegate void CallRecieved(Call^ call);
delegate void CallStatusUpdated(Call^ call);

namespace ViewModel {
public ref class CallsViewModel sealed
{
internal:
    /* singleton */
    static property CallsViewModel^ instance
    {
        CallsViewModel^ get()
        {
            static CallsViewModel^ instance_ = ref new CallsViewModel();
            return instance_;
        }
    }

    /* functions */
    Call^ addNewCall(String^ accountId, String^ callId, String^ from);
    void clearCallsList();
    void setState(String^ callId, String^ state, int code);

    /* properties */
    property Vector<Call^>^ CallsList
    {
        Vector<Call^>^ get()
        {
            return CallsList_;
        }
    }

    /* events */
    event CallRecieved^ callRecieved;
    event CallStatusUpdated^ callStatusUpdated;

private:
    CallsViewModel(); // singleton
    Vector<Call^>^ CallsList_;

};
}
}
