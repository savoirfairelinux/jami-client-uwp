/***************************************************************************
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
#include "pch.h"

/* daemon */
#include <dring.h>
#include "callmanager_interface.h"
#include "configurationmanager_interface.h"
#include "presencemanager_interface.h"
#include "fileutils.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Storage;
using namespace Windows::UI::Core;

using namespace RingClientUWP;

CoreDispatcher^ g_dispatcher;
void
DebugOutputWrapper(const std::string& str)
{
    MSG_(str);
}

void
RingClientUWP::RingD::startDaemon()
{
    if (daemonRunning_) {
        WNG_("\daemon already started.\n");
        return;
    }

    g_dispatcher = CoreApplication::MainView->CoreWindow->Dispatcher;

    daemonRunning_ = true;
    create_task([&]() {

        using SharedCallback = std::shared_ptr<DRing::CallbackWrapperBase>;

        /* ALPHABETICAL ORDER */
        std::map<std::string, SharedCallback> callHandlers = {
            DRing::exportable_callback<DRing::CallSignal::AudioMuted>([this](
                const std::string& callId,
                bool state)
            {
                MSG_("<AudioMuted>");
                MSG_("callId = " + callId);
                MSG_("state = " + (state) ? "true" : "false");
            }),
            DRing::exportable_callback<DRing::ConfigurationSignal::IncomingAccountMessage>([this](
                const std::string& accountId,
                const std::string& from,
                const std::map<std::string, std::string>& payloads)
            {
                MSG_("<IncomingAccountMessage>");
                MSG_("accountId = " + accountId);
                MSG_("from = " + from);

                for (auto i : payloads) {
                    MSG_("payload = " + i.second);
                    auto payload = Utils::toPlatformString(i.second);
                }
            }),
            // use IncomingCall only to register the call client sided, use StateChange to determine the impact on the UI
            DRing::exportable_callback<DRing::CallSignal::IncomingCall>([this](
                        const std::string& accountId,
                        const std::string& callId,
                        const std::string& from)
            {
                MSG_("<IncomingCall>");
                MSG_("accountId = " + accountId);
                MSG_("callId = " + callId);
                MSG_("from = " + from);
            }),
            DRing::exportable_callback<DRing::CallSignal::NewCallCreated>([this](
                        const std::string& accountId,
                        const std::string& callId,
                        const std::string& to)
            {
                MSG_("<NewCallCreated>");
                MSG_("accountId = " + accountId);
                MSG_("callId = " + callId);
                MSG_("to = " + to);
            }),
            DRing::exportable_callback<DRing::CallSignal::StateChange>([this](
                        const std::string& callId,
                        const std::string& state,
                        int code)
            {
                MSG_("<StateChange>");
                MSG_("callId = " + callId);
                MSG_("state = " + state);
                MSG_("code = " + std::to_string(code));
            }),
            DRing::exportable_callback<DRing::CallSignal::VideoMuted>([this](
                        const std::string& callId,
                        bool state)
            {
                MSG_("<VideoMuted>");
                MSG_("callId = " + callId);
                MSG_("state = " + (state) ? "true" : "false");
            })//,
        };

        registerCallHandlers(callHandlers);

        std::map<std::string, SharedCallback> dringDebugOut;
        dringDebugOut.insert(DRing::exportable_callback<DRing::Debug::MessageSend>
                             (std::bind(&DebugOutputWrapper, std::placeholders::_1)));
        registerCallHandlers(dringDebugOut);

        DRing::init(static_cast<DRing::InitFlag>(DRing::DRING_FLAG_CONSOLE_LOG |
                    DRing::DRING_FLAG_DEBUG |
                    DRing::DRING_FLAG_AUTOANSWER)
                    , localFolder_.c_str());

        MSG_("\ndaemon initialized.\n");

        MSG_("\nstarting daemon.\n");
        if (!DRing::start()) {
            ERR_("\ndaemon didn't start.\n");
            return;
        }
        MSG_("\ndaemon started.\n");

        MSG_("\nstarting daemon loop.\n");

        while (daemonRunning_) {
            DRing::pollEvents();

            Sleep(1000);
        }

        MSG_("\nstoping daemon.\n");
        DRing::fini();

        MSG_("\daemon stopped.\n");

    });
}

void RingClientUWP::RingD::stopDaemon()
{
    daemonRunning_ = false;
}

RingClientUWP::RingD::RingD()
{
    localFolder_ = Utils::toString(ApplicationData::Current->LocalFolder->Path);
    MSG_(localFolder_);
}
