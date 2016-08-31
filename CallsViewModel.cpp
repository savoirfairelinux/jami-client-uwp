/***************************************************************************
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

#include "CallsViewModel.h"

using namespace RingClientUWP;
using namespace ViewModel;
using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Core;

CallsViewModel::CallsViewModel()
{
    CallsList_ = ref new Vector<Call^>();

    /* connect to delegates. */

    RingD::instance->incomingCall += ref new RingClientUWP::IncomingCall([&](
    String^ accountId, String^ callId, String^ from) {
        auto call = addNewCall(accountId, callId, from);
        callRecieved(call);
    });

    RingD::instance->stateChange += ref new RingClientUWP::StateChange([&](
    String^ callId, String^ state, int code) {
        for each (auto call in CallsList_) {
            if (call->callId == callId) {
                call->stateChange(state, code);
                return;
            }
        }
        WNG_("Call not found");
    });
}

Call^
RingClientUWP::ViewModel::CallsViewModel::addNewCall(String^ accountId, String^ callId, String^ from)
{
    auto call = ref new Call(accountId, callId, from);
    CallsList_->Append(call);
    return call;
}

void RingClientUWP::ViewModel::CallsViewModel::clearCallsList()
{
    CallsList_->Clear();
}

void RingClientUWP::ViewModel::CallsViewModel::setState(String^ callId, String^ state, int code)
{
    1;
    auto jj = CallsList_->Size;
    auto kk = jj.ToString();
    auto nn = Utils::toString(kk);
    MSG_("is ="+nn);
    MSG_("aim = "+Utils::toString(callId));
    for each (auto call in CallsList_) {
        MSG_("x = "+Utils::toString(call->callId));
        if (call->callId == callId) {
            call->stateChange(state, code);
            // do something about the ui with the state
            return;
        }
    }
    WNG_("Call not found");
}
