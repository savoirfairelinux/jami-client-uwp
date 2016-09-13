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
        // REFACTO : add if call == nullptr
        callRecieved(call);
    });

    RingD::instance->stateChange += ref new RingClientUWP::StateChange([&](
    String^ callId, String^ state, int code) {
        for each (auto call in CallsList_) {
            if (call->callId == callId) {
                if (state == "OVER") {
                    delete call;
                    call->stateChange("", code);
                    callStatusUpdated(call); // used ?
                    RingD::instance->hangUpCall(call);
                    return;
                }
                call->stateChange(state, code);
                callStatusUpdated(call); // same...
                return;
            }
        }
        WNG_("Call not found");
    });
}

Call^
RingClientUWP::ViewModel::CallsViewModel::addNewCall(String^ accountId, String^ callId, String^ peer)
{
    if (accountId == "" | callId == "" | peer == "") {
        WNG_("call can't be created");
    }
    auto call = ref new Call(accountId, callId, peer);
    CallsList_->Append(call);
    return call;
}

void RingClientUWP::ViewModel::CallsViewModel::clearCallsList()
{
    CallsList_->Clear();
}

Call^
CallsViewModel::findCall(String^ callId)
{
    for each (Call^ call in CallsList_)
        if (call->callId == callId)
            return call;

    return nullptr;
}
