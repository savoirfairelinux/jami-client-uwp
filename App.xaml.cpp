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

#include "MainPage.xaml.h"
#include "Wizard.xaml.h"

//daemon
//#include <dring.h>
//#include "callmanager_interface.h"
//#include "configurationmanager_interface.h"
//#include "presencemanager_interface.h"
//#if RING_VIDEO
//#include "videomanager_interface.h"
//#endif
//#include "fileutils.h"

using namespace Concurrency;
using namespace Windows::Storage;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::ViewManagement;

using namespace RingClientUWP;

//CoreDispatcher^ g_dispatcher;
//std::string localFolder_;
//
//void
//DebugOutputWrapper(const std::string& str)
//{
//    MSG_(str);
//}

App::App()
{
    InitializeComponent(); // summon partial class, form generated files trough App.xaml

}

void
App::OnLaunched(LaunchActivatedEventArgs^ e)
{
    bool noAccountFound = true;
    rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

    if (rootFrame == nullptr) {
        rootFrame = ref new Frame();

        if (rootFrame->Content == nullptr)
            if(noAccountFound)
                rootFrame->Navigate(TypeName(Views::Wizard::typeid), e->Arguments);
            else
                rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);

        Window::Current->Content = rootFrame;
        Window::Current->Activate();
    } else if (noAccountFound)
        rootFrame->Navigate(TypeName(Views::Wizard::typeid), e->Arguments);
    else
        rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);

    CoreApplication::GetCurrentView()->TitleBar->ExtendViewIntoTitleBar = true;
    ApplicationView::GetForCurrentView()->TitleBar->ButtonBackgroundColor = Colors::LightBlue;
    ApplicationView::GetForCurrentView()->TitleBar->ButtonInactiveBackgroundColor = Colors::LightBlue;
    ApplicationView::GetForCurrentView()->TitleBar->ForegroundColor = Colors::White;
    ApplicationView::GetForCurrentView()->TitleBar->ButtonForegroundColor = Colors::White;

    /* summon the daemon */
    RingD::instance->startDaemon();

    //localFolder_ = Utils::toString(ApplicationData::Current->LocalFolder->Path);

    //StorageFolder^ localFolder = ApplicationData::Current->LocalFolder;
    //std::string localPath = Utils::toString(localFolder->Path);

    //create_task([this,localPath]() {

    //    using SharedCallback = std::shared_ptr<DRing::CallbackWrapperBase>;

    //    std::map<std::string, SharedCallback> callHandlers = {
    //        // use IncomingCall only to register the call client sided, use StateChange to determine the impact on the UI
    //        DRing::exportable_callback<DRing::CallSignal::IncomingCall>([this](
    //            const std::string& accountId,
    //            const std::string& callId,
    //            const std::string& from)
    //        {
    //            MSG_("<IncomingCall>");
    //            MSG_("accountId = " + accountId);
    //            MSG_("callId = " + callId);
    //            MSG_("from = " + from);
    //        }),
    //        DRing::exportable_callback<DRing::CallSignal::StateChange>([this](
    //            const std::string& callId,
    //            const std::string& state,
    //            int code)
    //        {
    //            MSG_("<StateChange>");
    //            MSG_("callId = " + callId);
    //            MSG_("state = " + state);
    //            MSG_("code = " + std::to_string(code));
    //        }),
    //        DRing::exportable_callback<DRing::ConfigurationSignal::IncomingAccountMessage>([this](
    //            const std::string& accountId,
    //            const std::string& from,
    //            const std::map<std::string, std::string>& payloads)
    //        {
    //            MSG_("<IncomingAccountMessage>");
    //            MSG_("accountId = " + accountId);
    //            MSG_("from = " + from);

    //            for (auto i : payloads) {
    //                MSG_("payload = " + i.second);
    //                auto payload = Utils::toPlatformString(i.second);
    //            }
    //        })
    //    };

    //    registerCallHandlers(callHandlers);

    //    std::map<std::string, SharedCallback> dringDebugOut;
    //    dringDebugOut.insert(DRing::exportable_callback<DRing::Debug::MessageSend>
    //                         (std::bind(&DebugOutputWrapper, std::placeholders::_1)));
    //    registerCallHandlers(dringDebugOut);

    //    DRing::init(static_cast<DRing::InitFlag>(DRing::DRING_FLAG_CONSOLE_LOG |
    //        DRing::DRING_FLAG_DEBUG |
    //        DRing::DRING_FLAG_AUTOANSWER)
    //                , localPath.c_str());

    //    MSG_("\ndaemon initialized.\n");

    //    MSG_("\nstarting daemon.\n");
    //    if (!DRing::start()) {
    //        ERR_("\ndaemon didn't start.\n");
    //        return;
    //    }
    //    MSG_("\ndaemon started.\n");

    //    MSG_("\nstarting daemon loop.\n");

    //    while (true) {
    //        DRing::pollEvents();

    //        Sleep(1000);
    //    }

    //    MSG_("\nstoping daemon.\n");
    //    DRing::fini();

    //    MSG_("\daemon stopped.\n");

    //});

}