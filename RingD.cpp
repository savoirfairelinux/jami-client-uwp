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
#include "pch.h"

/* daemon */
#include <dring.h>
#include "callmanager_interface.h"
#include "configurationmanager_interface.h"
#include "presencemanager_interface.h"
#include "videomanager_interface.h"
#include "fileutils.h"
#include "account_schema.h"
#include "account_const.h"

#include "SmartPanel.xaml.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Storage;
using namespace Windows::UI::Core;
using namespace Windows::Media;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;

using namespace RingClientUWP;
using namespace RingClientUWP::Utils;
using namespace RingClientUWP::ViewModel;

void
RingClientUWP::RingD::reloadAccountList()
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
            accountDetails.find(ring::Conf::CONFIG_ACCOUNT_TYPE)->second,       //type
            *rit);
    }
    // load user preferences
    Configuration::UserPreferences::instance->load();
}

/* nb: send message during conversation not chat video message */
void RingClientUWP::RingD::sendAccountTextMessage(String^ message)
{
    /* account id */
    auto accountId = AccountsViewModel::instance->selectedAccount->accountID_;
    std::wstring accountId2(accountId->Begin());
    std::string accountId3(accountId2.begin(), accountId2.end());

    /* recipient */
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = item->_contact;
    auto toRingId = contact->ringID_;
    std::wstring toRingId2(toRingId->Begin());
    std::string toRingId3(toRingId2.begin(), toRingId2.end());

    /* payload(s) */
    std::wstring message2(message->Begin());
    std::string message3(message2.begin(), message2.end());
    std::map<std::string, std::string> payloads;
    payloads["text/plain"] = message3;

    /* daemon */
    auto sent = DRing::sendAccountTextMessage(accountId3, toRingId3, payloads);

    /* conversation */
    if (sent) {
        contact->_conversation->addMessage(""/* date not yet used*/, MSG_FROM_ME, message);

        /* save contacts conversation to disk */
        contact->saveConversationToFile();

    } else {
        WNG_("message not sent, see daemon outputs");
    }
}

void
RingD::createRINGAccount(String^ alias)
{
    // refactoring : create a dedicated class constructor task and removes accountName from RingD
    accountName = Utils::toString(alias);
    tasksList_.push(ref new RingD::Task(Request::AddRingAccount));
}

void
RingD::createSIPAccount(String^ alias)
{
    // refactoring : create a dedicated class constructor task and removes accountName from RingD
    accountName = Utils::toString(alias);
    tasksList_.push(ref new RingD::Task(Request::AddSIPAccount));
}

void RingClientUWP::RingD::refuseIncommingCall(Call^ call)
{
    tasksList_.push(ref new RingD::Task(Request::RefuseIncommingCall, call));
}

void RingClientUWP::RingD::acceptIncommingCall(Call^ call)
{
    tasksList_.push(ref new RingD::Task(Request::AcceptIncommingCall, call));
}

void RingClientUWP::RingD::placeCall(Contact^ contact)
{
    MSG_("!--->> placeCall");
    auto to = contact->ringID_;
    auto accountId = AccountsViewModel::instance->selectedAccount->accountID_;

    auto to2 = Utils::toString(to);
    auto accountId2 = Utils::toString(accountId);

    auto callId2 = DRing::placeCall(accountId2, to2);



    if (callId2 == "") {
        WNG_("call not created, the daemon didn't return a call Id");
        return;
    }

    auto callId = Utils::toPlatformString(callId2);


    //auto con = ContactsViewModel::instance->findContactByName(to);
    auto item = SmartPanelItemsViewModel::instance->findItem(contact);
    item->_callId = callId;
    MSG_("$1 place call with id : " + Utils::toString(item->_callId));


    auto call = CallsViewModel::instance->addNewCall(accountId, callId, to);
    call->isOutGoing = true;

    if (call == nullptr) {
        WNG_("call not created, nullptr reason");
        return;
    }

    calling(call);

}

void
RingClientUWP::RingD::cancelOutGoingCall(Call^ call)
{
    MSG_("1!--->> cancelOutGoingCall");
    if (call)
        tasksList_.push(ref new RingD::Task(Request::CancelOutGoingCall, call));
}

void RingClientUWP::RingD::cancelOutGoingCall2(String ^ callId)
{
    MSG_("$1 cancelOutGoingCall2 : " + Utils::toString(callId));
    tasksList_.push(ref new RingD::Task(Request::HangUpCall, callId, 0));
}

void
RingClientUWP::RingD::hangUpCall(Call^ call)
{
    tasksList_.push(ref new RingD::Task(Request::HangUpCall, call));
}

void RingClientUWP::RingD::hangUpCall2(String ^ callId)
{
    MSG_("$1 hangUpCall2 : "+Utils::toString(callId));
    tasksList_.push(ref new RingD::Task(Request::HangUpCall, callId, 0));
}

void
RingClientUWP::RingD::startDaemon()
{
    // TODO (during refactoring) : use namespace
    /* clear the calls list and instantiate the singleton (required)  */
    RingClientUWP::ViewModel::CallsViewModel::instance->clearCallsList();

    create_task([&]()
    {
        using SharedCallback = std::shared_ptr<DRing::CallbackWrapperBase>;
        using namespace std::placeholders;

        auto dispatcher = CoreApplication::MainView->CoreWindow->Dispatcher;

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

                /* fix some issue in the daemon --> <...@...> */
                from2 = Utils::TrimRingId(from2);

                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                    CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
                {
                    incomingCall(accountId2, callId2, from2);
                    stateChange(callId2, CallStatus::INCOMING_RINGING, 0);


                    auto contact = ContactsViewModel::instance->findContactByName(from2);
                    auto item = SmartPanelItemsViewModel::instance->findItem(contact);
                    item->_callId = callId2;

                }));
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

                auto state3 = getCallStatus(state2);


                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                    CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
                {
                    stateChange(callId2, state3, code);
                }));
            }),
            DRing::exportable_callback<DRing::ConfigurationSignal::IncomingAccountMessage>([&](
                        const std::string& accountId,
                        const std::string& from,
                        const std::map<std::string, std::string>& payloads)
            {
                MSG_("<IncomingAccountMessage>");
                MSG_("accountId = " + accountId);
                MSG_("from = " + from);

                auto accountId2 = toPlatformString(accountId);
                auto from2 = toPlatformString(from);

                for (auto i : payloads) {
                    MSG_("payload = " + i.second);
                    auto payload = Utils::toPlatformString(i.second);
                    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                        CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
                    {
                        incomingAccountMessage(accountId2, from2, payload);
                    }));
                }
            }),
            DRing::exportable_callback<DRing::CallSignal::IncomingMessage>([&](
                        const std::string& callId,
                        const std::string& from,
                        const std::map<std::string, std::string>& payloads)
            {
                MSG_("<IncomingMessage>");
                MSG_("callId = " + callId);
                MSG_("from = " + from);

                auto callId2 = toPlatformString(callId);
                auto from2 = toPlatformString(from);

                from2 = Utils::TrimRingId2(from2);

                Call^ call = CallsViewModel::instance->findCall(callId2);

                if (!call)
                    return;

                String^ accountId2 = call->accountId;
                const std::string PROFILE_VCF = "x-ring/ring.profile.vcard";
                static const unsigned int profileSize = PROFILE_VCF.size();

                for (auto i : payloads) {
                    if (i.first.compare(0, profileSize, PROFILE_VCF) == 0) {
                        MSG_("VCARD");
                        return;
                    }
                    MSG_("payload.first = " + i.first);
                    MSG_("payload.second = " + i.second);

                    auto payload = Utils::toPlatformString(i.second);
                    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                        CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
                    {
                        incomingAccountMessage(accountId2, from2, payload);
                    }));
                }
            }),
            DRing::exportable_callback<DRing::ConfigurationSignal::RegistrationStateChanged>([this](
                        const std::string& account_id, const std::string& state,
                        int detailsCode, const std::string& detailsStr)
            {
                MSG_("<RegistrationStateChanged>: ID = " + account_id + "state = " + state);
                if (state == DRing::Account::States::REGISTERED) {
                    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                    ref new DispatchedHandler([=]() {
                        auto frame = dynamic_cast<Frame^>(Window::Current->Content);
                        dynamic_cast<RingClientUWP::MainPage^>(frame->Content)->showLoadingOverlay(false, false);
                    }));
                }
            }),
            DRing::exportable_callback<DRing::ConfigurationSignal::AccountsChanged>([this]()
            {
                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    reloadAccountList();
                }));
            }),
            DRing::exportable_callback<DRing::Debug::MessageSend>([&](const std::string& toto)
            {
                dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    RingDebug::instance->print(toto);
                }));
            })
        };
        registerCallHandlers(callHandlers);

        std::map<std::string, SharedCallback> getAppPathHandler =
        {
            DRing::exportable_callback<DRing::ConfigurationSignal::GetAppDataPath>
            ([this](std::vector<std::string>* paths) {
                paths->emplace_back(localFolder_);
            })
        };
        registerCallHandlers(getAppPathHandler);

        std::map<std::string, SharedCallback> incomingVideoHandlers =
        {
            DRing::exportable_callback<DRing::VideoSignal::DeviceEvent>
            ([this]() {
                MSG_("<DeviceEvent>");
            }),
            DRing::exportable_callback<DRing::VideoSignal::DecodingStarted>
            ([this](const std::string &id, const std::string &shmPath, int width, int height, bool isMixer) {
                MSG_("<DecodingStarted>");
                Video::VideoManager::instance->rendererManager()->startedDecoding(
                    Utils::toPlatformString(id),
                    width,
                    height);
            }),
            DRing::exportable_callback<DRing::VideoSignal::DecodingStopped>
            ([this](const std::string &id, const std::string &shmPath, bool isMixer) {
                MSG_("<DecodingStopped>");
                MSG_("Removing renderer id:" + id);
                /*auto Id = Utils::toPlatformString(id);
                auto renderer = Video::VideoManager::instance->rendererManager()->renderer(Id);
                if (renderer)
                    renderer->isRendering = false;*/
                Video::VideoManager::instance->rendererManager()->removeRenderer(Utils::toPlatformString(id));
            })
        };
        registerVideoHandlers(incomingVideoHandlers);

        using namespace Video;
        std::map<std::string, SharedCallback> outgoingVideoHandlers =
        {
            DRing::exportable_callback<DRing::VideoSignal::GetCameraInfo>
            ([this](const std::string& device,
            std::vector<std::string> *formats,
            std::vector<unsigned> *sizes,
            std::vector<unsigned> *rates) {
                MSG_("\n<GetCameraInfo>\n");
                auto device_list = VideoManager::instance->captureManager()->deviceList;

                for (unsigned int i = 0; i < device_list->Size; i++) {
                    auto dev = device_list->GetAt(i);
                    if (device == Utils::toString(dev->name())) {
                        auto channel = dev->channel();
                        Vector<Video::Resolution^>^ resolutions = channel->resolutionList();
                        for (auto res : resolutions) {
                            formats->emplace_back(Utils::toString(res->format()));
                            sizes->emplace_back(res->size()->width());
                            sizes->emplace_back(res->size()->height());
                            rates->emplace_back(res->activeRate()->value());
                        }
                    }
                }
            }),
            DRing::exportable_callback<DRing::VideoSignal::SetParameters>
            ([this](const std::string& device,
                    std::string format,
                    const int width,
                    const int height,
            const int rate) {
                MSG_("\n<SetParameters>\n");
                VideoManager::instance->captureManager()->activeDevice->SetDeviceProperties(
                    Utils::toPlatformString(format),width,height,rate);
            }),
            DRing::exportable_callback<DRing::VideoSignal::StartCapture>
            ([&](const std::string& device) {
                MSG_("\n<StartCapture>\n");
                dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    VideoManager::instance->captureManager()->InitializeCameraAsync();
                    VideoManager::instance->captureManager()->videoFrameCopyInvoker->Start();
                }));
            }),
            DRing::exportable_callback<DRing::VideoSignal::StopCapture>
            ([&]() {
                MSG_("\n<StopCapture>\n");
                dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    VideoManager::instance->captureManager()->StopPreviewAsync();
                    if (VideoManager::instance->captureManager()->captureTaskTokenSource)
                        VideoManager::instance->captureManager()->captureTaskTokenSource->cancel();
                    VideoManager::instance->captureManager()->videoFrameCopyInvoker->Stop();
                }));
            })
        };
        registerVideoHandlers(outgoingVideoHandlers);

        DRing::init(static_cast<DRing::InitFlag>(DRing::DRING_FLAG_CONSOLE_LOG |
                    DRing::DRING_FLAG_DEBUG));

        if (!DRing::start()) {
            ERR_("\ndaemon didn't start.\n");
            return;
        }
        else {
            if (!hasConfig) {
                tasksList_.push(ref new RingD::Task(Request::AddRingAccount));
                tasksList_.push(ref new RingD::Task(Request::AddSIPAccount));
            }
            else {
                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    reloadAccountList();
                }));
            }
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
        auto request = dynamic_cast<Task^>(task)->request;
        switch (request) {
        case Request::None:
            break;
        case Request::AddRingAccount:
        {
            std::map<std::string, std::string> ringAccountDetails;
            ringAccountDetails.insert(std::make_pair(ring::Conf::CONFIG_ACCOUNT_ALIAS, accountName));
            ringAccountDetails.insert(std::make_pair(ring::Conf::CONFIG_ACCOUNT_TYPE,"RING"));
            DRing::addAccount(ringAccountDetails);
        }
        break;
        case Request::AddSIPAccount:
        {
            std::map<std::string, std::string> sipAccountDetails;
            sipAccountDetails.insert(std::make_pair(ring::Conf::CONFIG_ACCOUNT_ALIAS, accountName + " (SIP)"));
            sipAccountDetails.insert(std::make_pair(ring::Conf::CONFIG_ACCOUNT_TYPE,"SIP"));
            DRing::addAccount(sipAccountDetails);
        }
        break;
        case Request::RefuseIncommingCall:
        {
            auto callId = task->_call->callId;
            auto callId2 = Utils::toString(callId);
            DRing::refuse(callId2);
        }
        break;
        case Request::AcceptIncommingCall:
        {
            auto callId = task->_call->callId;
            auto callId2 = Utils::toString(callId);
            DRing::accept(callId2);
        }
        break;
        case Request::CancelOutGoingCall:
        case Request::HangUpCall:
        {

            MSG_("1!--->> Request::CancelOutGoingCall");
            auto id = task->_callId;
            DRing::hangUp(Utils::toString(id));
            return;

            auto callId = task->_call->callId;
            auto callId2 = Utils::toString(callId);
            DRing::hangUp(callId2);
        }
        break;
        default:
            break;
        }
        tasksList_.pop();
    }
}

CallStatus RingClientUWP::RingD::getCallStatus(String^ state)
{
    if (state == "INCOMING")
        return CallStatus::INCOMING_RINGING;

    if (state == "CURRENT")
        return CallStatus::IN_PROGRESS;

    if (state == "OVER")
        return CallStatus::ENDED;

    if (state == "RINGING")
        return CallStatus::OUTGOING_RINGING;

    if (state == "CONNECTING")
        return CallStatus::SEARCHING;

    return CallStatus::NONE;
}
