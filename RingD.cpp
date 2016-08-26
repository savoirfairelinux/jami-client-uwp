/***************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: J�ger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
#include "pch.h"

/* daemon */
#include <dring.h>
#include "callmanager_interface.h"
#include "configurationmanager_interface.h"
#include "presencemanager_interface.h"
#include "fileutils.h"

#include "account_schema.h"

#include "SmartPanel.xaml.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Storage;
using namespace Windows::UI::Core;

using namespace RingClientUWP;
using namespace RingClientUWP::Utils;

void
debugOutputWrapper(const std::string& str)
{
    MSG_(str);
}

void
reloadAccountList()
{
    RingClientUWP::ViewModel::AccountsViewModel::instance->clearAccountList();
    std::vector<std::string> accountList = DRing::getAccountList();
    std::vector<std::string>::reverse_iterator rit = accountList.rbegin();
    for (; rit != accountList.rend(); ++rit) {
        std::map<std::string,std::string> accountDetails = DRing::getAccountDetails(*rit);
        std::string ringID(accountDetails.find(ring::Conf::CONFIG_ACCOUNT_USERNAME)->second);
        if(!ringID.empty())
            ringID = ringID.substr(5);
        RingClientUWP::ViewModel::AccountsViewModel::instance->add(
            accountDetails.find(ring::Conf::CONFIG_ACCOUNT_ALIAS)->second,      //name
            ringID,                                                             //ringid
            accountDetails.find(ring::Conf::CONFIG_ACCOUNT_TYPE)->second);      //type
    }
}

void
RingClientUWP::RingD::startDaemon()
{
    create_task([&]()
    {
        using SharedCallback = std::shared_ptr<DRing::CallbackWrapperBase>;
        using namespace std::placeholders;

        std::map<std::string, SharedCallback> callHandlers = {
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

                auto accountId2 = toPlatformString(accountId);
                auto callId2 = toPlatformString(callId);
                auto from2 = toPlatformString(from);

                incomingCall(accountId2, callId2, from2);

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

                auto callId2 = toPlatformString(callId);
                auto state2 = toPlatformString(state);

                stateChange(callId2, state2, code);

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
            DRing::exportable_callback<DRing::ConfigurationSignal::AccountsChanged>([this]()
            {
                reloadAccountList();
            })
        };

        registerCallHandlers(callHandlers);

        std::map<std::string, SharedCallback> dringDebugOutHandler;
        dringDebugOutHandler.insert(DRing::exportable_callback<DRing::Debug::MessageSend>
                                    (std::bind(&debugOutputWrapper, _1)));
        registerCallHandlers(dringDebugOutHandler);

        std::map<std::string, SharedCallback> getAppPathHandler =
        {
            DRing::exportable_callback<DRing::ConfigurationSignal::GetAppDataPath>
            ([this](std::vector<std::string>* paths) {
                paths->emplace_back(localFolder_);
            })
        };
        registerCallHandlers(getAppPathHandler);

        DRing::init(static_cast<DRing::InitFlag>(DRing::DRING_FLAG_CONSOLE_LOG |
                    DRing::DRING_FLAG_DEBUG |
                    DRing::DRING_FLAG_AUTOANSWER));

        if (!DRing::start()) {
            ERR_("\ndaemon didn't start.\n");
            return;
        }
        else {
            if (!hasConfig)
            {
                std::map<std::string, std::string> ringAccountDetails;
                ringAccountDetails.insert(std::make_pair(ring::Conf::CONFIG_ACCOUNT_ALIAS, accountName));
                ringAccountDetails.insert(std::make_pair(ring::Conf::CONFIG_ACCOUNT_TYPE,"RING"));
                DRing::addAccount(ringAccountDetails);
            }
            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
               ref new DispatchedHandler([=]() {
                reloadAccountList();
            }));
            while (true) {
                DRing::pollEvents();
                Sleep(1000);
                dequeueTasks();
            }
            DRing::fini();
        }
    });
}

RingD::RingD()
{
    localFolder_ = Utils::toString(ApplicationData::Current->LocalFolder->Path);
}

void
RingD::dequeueTasks()
{
    for (int i = 0; i < tasksList_.size(); i++) {
        auto task = tasksList_.front();
        switch (task->request) {
        case Request::None:
        default:
            break;
        }
        tasksList_.pop();
    }
}
