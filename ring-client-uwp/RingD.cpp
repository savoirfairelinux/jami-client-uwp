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

#include "RingD.h"

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

void
DebugOutputWrapper(const std::string& str) // global
{
    CoreApplication::MainView->CoreWindow->Dispatcher
    ->RunAsync(CoreDispatcherPriority::Low, ref new DispatchedHandler([str]()
    {
        RingDebug::instance->print(str);
    }));
}

void
RingClientUWP::RingD::startDaemon()
{
    if (daemonRunning_)
        return;

    daemonRunning_ = true;
    create_task([&]() {
        CoreApplication::MainView->CoreWindow->Dispatcher
        ->RunAsync(CoreDispatcherPriority::Low, ref new DispatchedHandler([&]()
        {
            MSG_("\ninitializing...\n");
            MSG_("LocalFolder : " + localFolder_);
        }));

        using SharedCallback = std::shared_ptr<DRing::CallbackWrapperBase>;

        std::map<std::string, SharedCallback> dringDebugOut;
        dringDebugOut.insert(DRing::exportable_callback<DRing::Debug::MessageSend>
                             (std::bind(&DebugOutputWrapper, std::placeholders::_1)));
        registerCallHandlers(dringDebugOut);

        DRing::init(static_cast<DRing::InitFlag>(DRing::DRING_FLAG_CONSOLE_LOG | DRing::DRING_FLAG_DEBUG
                    /*| DRing::DRING_FLAG_AUTOANSWER*/), localFolder_.c_str());

        while (daemonRunning_) {
            //DRing::pollEvents();

            Sleep(1000);
        }

        DRing::fini();
        CoreApplication::MainView->CoreWindow->Dispatcher
        ->RunAsync(CoreDispatcherPriority::Low, ref new DispatchedHandler([&]()
        {
            MSG_("\norder to stop the daemon sent.\n");
        }));

    });
}

void RingClientUWP::RingD::stopDaemon()
{
    daemonRunning_ = false;
}



RingClientUWP::RingD::RingD()
{
    localFolder_ = Utils::toString(ApplicationData::Current->LocalFolder->Path);
}
