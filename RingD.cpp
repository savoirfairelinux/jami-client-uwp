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
#include "dring/call_const.h"

#include "callmanager_interface.h"
#include "configurationmanager_interface.h"
#include "presencemanager_interface.h"
#include "videomanager_interface.h"
#include "fileutils.h"
#include "account_schema.h"
#include "account_const.h"
#include "string_utils.h" // used to get some const expr like TRUE_STR
#include "gnutls\gnutls.h"
#include "media_const.h" // used to get some const expr like MEDIA_TYPE_VIDEO

#include "SmartPanel.xaml.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Storage;
using namespace Windows::UI::Core;
using namespace Windows::Media;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;
using namespace Windows::System::Threading;
using namespace Windows::Globalization::DateTimeFormatting;
using namespace Windows::UI::ViewManagement;
using namespace Windows::System;

using namespace RingClientUWP;
using namespace RingClientUWP::Utils;
using namespace RingClientUWP::ViewModel;

void
RingClientUWP::RingD::getAccounts()
{
    MSG_("loading accounts...");

    std::vector<std::string> accountList = DRing::getAccountList();

    /* if for any reason there is no account at all, screen the wizard */
    if (accountList.size() == 0) {
        summonWizard();
        return;
    }

    std::vector<std::string>::reverse_iterator rit = accountList.rbegin();

    for (; rit != accountList.rend(); ++rit) {
        std::map<std::string,std::string> accountDetails = DRing::getAccountDetails(*rit);
        auto accountId = *rit;
        auto type = accountDetails.find(DRing::Account::ConfProperties::TYPE)->second;
        if (type == "RING") {
            auto  ringID = accountDetails.find(DRing::Account::ConfProperties::USERNAME)->second;
            if (!ringID.empty())
                ringID = ringID.substr(5);
            bool active = (accountDetails.find(DRing::Account::ConfProperties::ENABLED)->second == ring::TRUE_STR)
                                ? true
                                : false;
            bool upnpState = (accountDetails.find(DRing::Account::ConfProperties::UPNP_ENABLED)->second == ring::TRUE_STR)
                                ? true
                                : false;
            bool autoAnswer = (accountDetails.find(DRing::Account::ConfProperties::AUTOANSWER)->second == ring::TRUE_STR)
                                ? true
                                : false;
            bool dhtPublicInCalls = (accountDetails.find(DRing::Account::ConfProperties::DHT::PUBLIC_IN_CALLS)->second == ring::TRUE_STR)
                                ? true
                                : false;
            auto alias = accountDetails.find(DRing::Account::ConfProperties::ALIAS)->second;
            auto deviceId = accountDetails.find(DRing::Account::ConfProperties::RING_DEVICE_ID)->second;

            auto account = AccountsViewModel::instance->findItem(Utils::toPlatformString(accountId));

            if (account) {
                account->name_ = Utils::toPlatformString(alias);
                account->_upnpState = upnpState;
                account->accountType_ = Utils::toPlatformString(type);
                account->ringID_ = Utils::toPlatformString(ringID);
                // load contact requests for the account
                auto contactRequests = DRing::getTrustRequests(accountId);
                if (auto contactListModel = AccountsViewModel::instance->getContactListModel(std::string(accountId))) {
                    for (auto& cr: contactRequests) {
                        auto ringId = cr.at("from");
                        auto timeReceived = cr.at("received");
                        auto payload = cr.at("payload");
                        auto fromP = Utils::toPlatformString(ringId);
                        auto contact = contactListModel->findContactByRingId(fromP);
                        if (contact == nullptr) {
                            contact = contactListModel->addNewContact(fromP, fromP, TrustStatus::INCOMING_CONTACT_REQUEST);

                            contactListModel->saveContactsToFile();
                            AccountsViewModel::instance->raiseUnreadContactRequest();

                            SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                            SmartPanelItemsViewModel::instance->update();
                        }
                        if (contact != nullptr && ContactRequestItemsViewModel::instance->findItem(contact) == nullptr) {
                            // The visible ring id will potentially be replaced by a username after a lookup
                            RingD::instance->lookUpAddress(accountId, Utils::toPlatformString(ringId));

                            auto vcard = contact->getVCard();
                            auto parsedPayload = VCardUtils::parseContactRequestPayload(payload);
                            vcard->setData(parsedPayload.at("VCARD"));
                            vcard->completeReception();

                            // The name is the ring id for now
                            contact->_name = Utils::toPlatformString(ringId);
                            contact->_displayName = Utils::toPlatformString(vcard->getPart("FN"));

                            auto newContactRequest = ref new ContactRequestItem();
                            newContactRequest->_contact = contact;
                            ContactRequestItemsViewModel::instance->itemsList->InsertAt(0, newContactRequest);

                            ContactRequestItemsViewModel::instance->refreshFilteredItemsList();
                            ContactRequestItemsViewModel::instance->update();
                        }
                    }
                }
                accountUpdated(account);
            }
            else {
                if (!ringID.empty())
                    AccountsViewModel::instance->addRingAccount(alias,
                                                                ringID,
                                                                accountId,
                                                                deviceId,
                                                                active,
                                                                upnpState,
                                                                autoAnswer,
                                                                dhtPublicInCalls);
            }
        }
        else { /* SIP */
            auto alias = accountDetails.find(DRing::Account::ConfProperties::ALIAS)->second;
            auto accountId = *rit;
            bool active = (accountDetails.find(DRing::Account::ConfProperties::ENABLED)->second == ring::TRUE_STR)
                            ? true
                            : false;
            auto sipHostname = accountDetails.find(DRing::Account::ConfProperties::HOSTNAME)->second;
            auto sipUsername = accountDetails.find(DRing::Account::ConfProperties::USERNAME)->second;
            auto sipPassword = accountDetails.find(DRing::Account::ConfProperties::PASSWORD)->second;

            auto account = AccountsViewModel::instance->findItem(Utils::toPlatformString(accountId));

            if (account) {
                account->name_ = Utils::toPlatformString(alias);
                account->accountType_ = Utils::toPlatformString(type);
                account->_sipHostname = Utils::toPlatformString(sipHostname);
                account->_sipUsername = Utils::toPlatformString(sipUsername);
                account->_sipPassword = Utils::toPlatformString(sipPassword);
                accountUpdated(account);
            }
            else {
                AccountsViewModel::instance->addSipAccount( alias,
                                                            accountId,
                                                            active,
                                                            sipHostname,
                                                            sipUsername,
                                                            sipPassword );
            }

            sipPassword.clear();
        }
    }

    for (auto account: AccountsViewModel::instance->accountsList) {
        if (Utils::hasInternet() && account->accountType_ == "SIP") {
            auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(account->accountID_));
            for (auto contact : contactListModel->_contactsList) {
                DRing::subscribeBuddy(Utils::toString(account->accountID_), Utils::toString(contact->ringID_), true);
            }
        }
    }

    // load user preferences
    Configuration::UserPreferences::instance->load();
}

/* nb: send message during conversation not chat video message */
void RingClientUWP::RingD::sendAccountTextMessage(String^ message)
{
    /* recipient */
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = item->_contact;
    auto toRingId = contact->ringID_;
    std::wstring toRingId2(toRingId->Begin());
    std::string toRingId3(toRingId2.begin(), toRingId2.end());

    /* account id */
    auto accountId = contact->_accountIdAssociated;
    std::wstring accountId2(accountId->Begin());
    std::string accountId3(accountId2.begin(), accountId2.end());

    /* payload(s) */
    std::wstring message2(message->Begin());
    std::string message3(message2.begin(), message2.end());
    std::map<std::string, std::string> payloads;
    payloads["text/plain"] = message3;

    /* daemon */
    auto sentToken = DRing::sendAccountTextMessage(accountId3, toRingId3, payloads);

    /* conversation */
    if (sentToken) {
        contact->_conversation->addMessage(MSG_FROM_ME, message, std::time(nullptr), false, sentToken.ToString());

        /* save contacts conversation to disk */
        contact->saveConversationToFile();
    } else {
        WNG_("message not sent, see daemon outputs");
    }
}

// send message during video call
void RingClientUWP::RingD::sendSIPTextMessage(String^ message)
{
    /* account id */
    auto accountId = AccountListItemsViewModel::instance->_selectedItem->_account->accountID_;
    std::wstring accountId2(accountId->Begin());
    std::string accountId3(accountId2.begin(), accountId2.end());

    /* call */
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto callId = item->_callId;
    std::wstring callId2(callId->Begin());
    std::string callId3(callId2.begin(), callId2.end());

    /* recipient */
    auto contact = item->_contact;

    /* payload(s) */
    std::wstring message2(message->Begin());
    std::string message3(message2.begin(), message2.end());
    std::map<std::string, std::string> payloads;
    payloads["text/plain"] = message3;

    /* daemon */
    DRing::sendTextMessage(callId3, payloads, accountId3, true /*not used*/);

    // No id generated for sip messages within a conversation, so let's generate one
    // so we can track message order.
    auto messageId = Utils::toPlatformString(Utils::genID(0LL, 9999999999999999999LL));
    contact->_conversation->addMessage(MSG_FROM_ME, message, std::time(nullptr), false, messageId);

    /* save contacts conversation to disk */
    contact->saveConversationToFile();
}

// send vcard
void RingClientUWP::RingD::sendSIPTextMessageVCF(std::string callID, std::map<std::string, std::string> message)
{
    /* account id */
    auto accountId = AccountListItemsViewModel::instance->_selectedItem->_account->accountID_;
    std::wstring accountId2(accountId->Begin());
    std::string accountId3(accountId2.begin(), accountId2.end());

    /* daemon */
    DRing::sendTextMessage(callID, message, accountId3, true /*not used*/);
}

void
RingD::createRINGAccount(String^ alias, String^ archivePassword, bool upnp, String^ registeredName)
{
    editModeOn_ = true;

    auto frame = dynamic_cast<Frame^>(Window::Current->Content);
    setLoadingStatusText("Creating account...", "#ff00ff00");
    dynamic_cast<RingClientUWP::MainPage^>(frame->Content)->showLoadingOverlay(true, true);

    auto task = ref new RingD::Task(Request::AddRingAccount);

    task->_alias = alias;
    task->_password = archivePassword;
    task->_upnp = upnp;
    task->_registeredName = registeredName;

    tasksList_.push(task);
}

void
RingD::createSIPAccount(String^ alias, String^ sipPassword, String^ sipHostname, String^ sipusername)
{
    editModeOn_ = true;

    auto frame = dynamic_cast<Frame^>(Window::Current->Content);

    auto task = ref new RingD::Task(Request::AddSIPAccount);

    task->_alias = alias;
    task->_sipPassword = sipPassword;
    task->_sipHostname = sipHostname;
    task->_sipUsername = sipusername;

    tasksList_.push(task);
}

void RingClientUWP::RingD::refuseIncommingCall(String^ callId)
{
    tasksList_.push(ref new RingD::Task(Request::RefuseIncommingCall, callId));
}

void RingClientUWP::RingD::acceptIncommingCall(String^ callId)
{
    tasksList_.push(ref new RingD::Task(Request::AcceptIncommingCall, callId));
}

void RingClientUWP::RingD::placeCall(Contact^ contact)
{
    MSG_("!--->> placeCall");

    if (contact->_accountIdAssociated->IsEmpty()) {
        MSG_("adding account id to contact");
        contact->_accountIdAssociated = AccountListItemsViewModel::instance->_selectedItem->_account->accountID_;
    }

    auto task = ref new RingD::Task(Request::PlaceCall);
    task->_accountId_new = Utils::toString(contact->_accountIdAssociated);
    task->_ringId_new = Utils::toString(contact->ringID_);
    task->_sipUsername = contact->_name;
    tasksList_.push(task);

}

void RingClientUWP::RingD::pauseCall(const std::string & callId)
{
    auto task = ref new RingD::Task(Request::PauseCall);
    task->_callid_new = callId;
    tasksList_.push(task);
}

void RingClientUWP::RingD::unPauseCall(const std::string & callId)
{
    auto task = ref new RingD::Task(Request::UnPauseCall);
    task->_callid_new = callId;
    tasksList_.push(task);
}

void RingClientUWP::RingD::raiseWindowResized()
{
    windowResized();
}

void RingClientUWP::RingD::cancelOutGoingCall2(String ^ callId)
{
    MSG_("$1 cancelOutGoingCall2 : " + Utils::toString(callId));
    tasksList_.push(ref new RingD::Task(Request::HangUpCall, callId));
}


void RingClientUWP::RingD::hangUpCall2(String ^ callId)
{
    MSG_("$1 hangUpCall2 : "+Utils::toString(callId));
    tasksList_.push(ref new RingD::Task(Request::HangUpCall, callId));
}

void RingClientUWP::RingD::pauseCall(String ^ callId) // do not use it, rm during refacto
{
    MSG_("$1 pauseCall : " + Utils::toString(callId));
    tasksList_.push(ref new RingD::Task(Request::PauseCall, callId));
}

void RingClientUWP::RingD::unPauseCall(String ^ callId) // do not use it, rm during refacto
{
    MSG_("$1 unPauseCall : " + Utils::toString(callId));
    tasksList_.push(ref new RingD::Task(Request::UnPauseCall, callId));
}

void RingClientUWP::RingD::askToRefreshKnownDevices(String^ accountId)
{
    auto task = ref new RingD::Task(Request::GetKnownDevices);
    task->_accountId = accountId;

    tasksList_.push(task);
}

void RingClientUWP::RingD::askToExportOnRing(String ^ accountId, String ^ password)
{
    auto task = ref new RingD::Task(Request::ExportOnRing);
    task->_accountId = accountId;
    task->_password = password;

    tasksList_.push(task);
}

void RingClientUWP::RingD::eraseCacheFolder()
{
    StorageFolder^ localFolder = ApplicationData::Current->LocalFolder;
    String^ folderName = ".cache";

    task<IStorageItem^>(localFolder->TryGetItemAsync(folderName)).then([this](IStorageItem^ folder)
    {
        if (folder) {
            MSG_("erasing cache folder.");
            folder->DeleteAsync();
        }
        else {
            WNG_("cache folder not found.");
        }
    });
}

void RingClientUWP::RingD::updateAccount(String^ accountId)
{
    editModeOn_ = true;

    auto frame = dynamic_cast<Frame^>(Window::Current->Content);

    auto task = ref new RingD::Task(Request::UpdateAccount);
    task->_accountId_new = Utils::toString(accountId);

    tasksList_.push(task);
}

void RingClientUWP::RingD::deleteAccount(String ^ accountId)
{
    editModeOn_ = true;

    auto frame = dynamic_cast<Frame^>(Window::Current->Content);

    auto task = ref new RingD::Task(Request::DeleteAccount);
    task->_accountId = accountId;

    tasksList_.push(task);
}

void RingClientUWP::RingD::registerThisDevice(String ^ pin, String ^ archivePassword)
{
    tasksList_.push(ref new RingD::Task(Request::RegisterDevice, pin, archivePassword));
    archivePassword = "";
    pin = "";
}

void
ShowCallToast(String^ callId, String^ from = nullptr)
{
    String^ payload =
        "<toast scenario='incomingCall'> "
            "<visual> "
                "<binding template='ToastGeneric'>"
                    "<text>GNU Ring - Incoming call"+ (from?(" from " + from):"") +"</text>"
                "</binding>"
            "</visual>"
            /*"<actions>"
                "<action arguments = '" + callId + "' content = 'Accept' />"
            "</actions>"*/
            "<audio src='ms-appx:///Assets/default.wav' loop='true'/>"
        "</toast>";

    auto doc = ref new XmlDocument();
    doc->LoadXml(payload);

    auto toast = ref new ToastNotification(doc);
    ToastNotificationManager::CreateToastNotifier()->Show(toast);
}

void
ShowMsgToast(String^ from, String^ payload)
{
    String^ xml =
        "<toast scenario='incomingMessage'> "
            "<visual> "
                "<binding template='ToastGeneric'>"
                    "<text>" + from + " : " + payload + "</text>"
                "</binding>"
            "</visual>"
            "<actions>"
                "<action arguments = '" + from + "'/>"
            "</actions>"
            "<audio src='ms-appx:///Assets/message_notification_sound.wav' loop='false'/>"
        "</toast>";

    auto doc = ref new XmlDocument();
    doc->LoadXml(xml);

    auto toast = ref new ToastNotification(doc);
    ToastNotificationManager::CreateToastNotifier()->Show(toast);
}

void
RingD::handleIncomingMessage(   const std::string& callId,
                                const std::string& accountId,
                                const std::string& from,
                                const std::map<std::string, std::string>& payloads)
{
    auto callId2 = toPlatformString(callId);
    auto accountId2 = toPlatformString(accountId);
    auto from2 = toPlatformString(from);
    from2 = Utils::TrimRingId2(from2);

    auto item = SmartPanelItemsViewModel::instance->findItemByRingID(from2);
    Contact^ contact;

    static const unsigned int profileSize = VCardUtils::PROFILE_VCF.size();
    for (auto i : payloads) {
        if (i.first.compare(0, profileSize, VCardUtils::PROFILE_VCF) == 0) {
            if (item) {
                contact = item->_contact;
                contact->getVCard()->receiveChunk(i.first, i.second);
            }
            else
                WNG_("item not found!");
            return;
        }
        auto payload = Utils::toPlatformString(i.second);
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
            CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
        {
            if (!accountId2->IsEmpty())
                incomingAccountMessage(accountId2, from2, payload);
            else if (!callId2->IsEmpty())
                incomingMessage(callId2, payload);
        }));
    }
}

void
RingD::registerCallbacks()
{
    dispatcher = CoreApplication::MainView->CoreWindow->Dispatcher;

    callHandlers = {
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
                auto account = AccountListItemsViewModel::instance->findItem(accountId2)->_account;
                if (auto contactListModel = AccountsViewModel::instance->getContactListModel(std::string(accountId))) {
                    Contact^ contact;
                    if (account->accountType_ == "RING")
                        contact = contactListModel->findContactByRingId(from2);
                    else if (account->accountType_ == "SIP")
                        contact = contactListModel->findContactByName(from2);
                    if (contact) {
                        auto item = SmartPanelItemsViewModel::instance->findItem(contact);
                        if (item)
                            item->_callId = callId2;
                    }
                }

                if (account->_autoAnswer) {
                    acceptIncommingCall(callId2);
                    stateChange(callId2, CallStatus::AUTO_ANSWERING, 0);
                }
                else {
                    incomingCall(accountId2, callId2, from2);
                    stateChange(callId2, CallStatus::INCOMING_RINGING, 0);
                }
            }));
        }),
        DRing::exportable_callback<DRing::CallSignal::PeerHold>([this](
                    const std::string& callId,
                    bool state)
        {
            // why does this callback exist ? why are we not using stateChange ?
            MSG_("<PeerHold>");
            MSG_("callId = " + callId);
            MSG_("state = " + Utils::toString(state.ToString()));

            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
            {
                if (state)
                    stateChange(Utils::toPlatformString(callId), CallStatus::PEER_PAUSED, 0);
                else
                    stateChange(Utils::toPlatformString(callId), CallStatus::IN_PROGRESS, 0);
            }));
        }),
        DRing::exportable_callback<DRing::CallSignal::AudioMuted>([this](
                    const std::string& callId,
                    bool state)
        {
            // why does this callback exist ? why are we not using stateChange ?
            MSG_("<AudioMuted>");
            MSG_("callId = " + callId);
            MSG_("state = " + Utils::toString(state.ToString()));

            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
            {
                audioMuted(callId, state);
            }));
        }),
        DRing::exportable_callback<DRing::CallSignal::VideoMuted>([this](
                    const std::string& callId,
                    bool state)
        {
            // why this cllaback exist ? why are we not using stateChange ?
            MSG_("<VideoMuted>");
            MSG_("callId = " + callId);
            MSG_("state = " + Utils::toString(state.ToString()));

            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
            {
                videoMuted(callId, state);
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

            auto item = SmartPanelItemsViewModel::instance->findItem(Utils::toPlatformString(callId));
            if (item == nullptr)
                return;

            auto callId2 = toPlatformString(callId);
            auto state2 = toPlatformString(state);

            auto state3 = translateCallStatus(state2);

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

            handleIncomingMessage("", accountId, from, payloads);
        }),
        DRing::exportable_callback<DRing::CallSignal::IncomingMessage>([&](
                    const std::string& callId,
                    const std::string& from,
                    const std::map<std::string, std::string>& payloads)
        {
            MSG_("<IncomingMessage>");
            MSG_("callId = " + callId);
            MSG_("from = " + from);

            handleIncomingMessage(callId, "", from, payloads);
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::RegistrationStateChanged>([this](
                    const std::string& account_id, const std::string& state,
                    int detailsCode, const std::string& detailsStr)
        {
            MSG_("<RegistrationStateChanged>: ID = " + account_id + " state = " + state);
            if (state == DRing::Account::States::REGISTERED) {
                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    if (auto account = AccountsViewModel::instance->findItem(Utils::toPlatformString(account_id))) {
                        account->_registrationState = RegistrationState::REGISTERED;
                        auto contactListModel = AccountsViewModel::instance->getContactListModel(Utils::toString(account->accountID_));
                        for (auto contact : contactListModel->_contactsList) {
                            MSG_("account: " + account->accountID_ + " subscribing to buddy presence for ringID: " + contact->ringID_);
                            DRing::subscribeBuddy(Utils::toString(account->accountID_), Utils::toString(contact->ringID_), true);
                        }
                    }
                    getAccounts();
                    registrationStateRegistered(account_id);
                }));
            }
            else if (state == DRing::Account::States::UNREGISTERED) {
                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                    ref new DispatchedHandler([=]() {
                    if (auto account = AccountsViewModel::instance->findItem(Utils::toPlatformString(account_id)))
                        account->_registrationState = RegistrationState::UNREGISTERED;
                    getAccounts();
                    registrationStateUnregistered(account_id);
                }));
            }
            else if (state == DRing::Account::States::TRYING) {
                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    if(auto account = AccountsViewModel::instance->findItem(Utils::toPlatformString(account_id)))
                        account->_registrationState = RegistrationState::TRYING;
                    getAccounts();
                    registrationStateTrying(account_id);
                }));
            }
            else if (state == DRing::Account::States::ERROR_GENERIC
                     || state == DRing::Account::States::ERROR_AUTH
                     || state == DRing::Account::States::ERROR_NETWORK
                     || state == DRing::Account::States::ERROR_HOST
                     || state == DRing::Account::States::ERROR_CONF_STUN
                     || state == DRing::Account::States::ERROR_EXIST_STUN
                     || state == DRing::Account::States::ERROR_SERVICE_UNAVAILABLE
                     || state == DRing::Account::States::ERROR_NOT_ACCEPTABLE
                     || state == DRing::Account::States::ERROR_NEED_MIGRATION
                     || state == DRing::Account::States::REQUEST_TIMEOUT) {
                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    getAccounts();
                    registrationStateErrorGeneric(account_id);
                }));
            }
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::AccountsChanged>([this]()
        {
            MSG_("<AccountsChanged>");
            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                getAccounts();
            }));
        }),
        DRing::exportable_callback<DRing::DebugSignal::MessageSend>([&](const std::string& msg)
        {
            if (debugModeOn_) {
                dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    DMSG_(msg);
                }));
            }
        }),
            DRing::exportable_callback<DRing::ConfigurationSignal::AccountMessageStatusChanged>([this](
                const std::string& account_id, uint64_t message_id, const std::string& to, int state)
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                MSG_("<AccountMessageStatusChanged>");
                MSG_("account_id = " + account_id);
                MSG_("message_id = " + message_id);
                MSG_("to = " + Utils::toPlatformString(to));
                MSG_("state = " + state.ToString());

                // acquittement de message
                auto contactListModel = AccountsViewModel::instance->getContactListModel(std::string(account_id));
                if (auto contact = contactListModel->findContactByRingId(Utils::toPlatformString(to))) {
                    auto conversation = contact->_conversation;
                    if (conversation) {
                        for (const auto& msg : conversation->_messages) {
                            if (msg->MessageIdInteger == message_id &&
                                state == Utils::toUnderlyingValue(DRing::Account::MessageStates::SENT)) {
                                // message has arrived
                                messageStatusUpdated(msg->MessageId, state);
                                msg->IsReceived = true;
                                contact->saveConversationToFile();
                            }
                        }
                    }
                }
            }));
        })
    };
    registerCallHandlers(callHandlers);

    getAppPathHandler =
    {
        DRing::exportable_callback<DRing::ConfigurationSignal::GetAppDataPath>
        ([this](const std::string& name, std::vector<std::string>* paths) {
            paths->emplace_back(localFolder_);
        })
    };
    registerCallHandlers(getAppPathHandler);

    /*getAppUserNameHandler =
    {
        DRing::exportable_callback<DRing::ConfigurationSignal::GetAppUserName>
        ([this](std::vector<std::string>* unames) {
            unames->emplace_back(Utils::toString(
                UserModel::instance->firstName +
                "." +
                UserModel::instance->lastName));
        })
    };
    registerCallHandlers(getAppUserNameHandler);*/

    incomingVideoHandlers =
    {
        DRing::exportable_callback<DRing::VideoSignal::DeviceEvent>
        ([this]() {
            MSG_("<DeviceEvent>");
        }),
        DRing::exportable_callback<DRing::VideoSignal::DecodingStarted>
        ([&](const std::string &id, const std::string &shmPath, int width, int height, bool isMixer) {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                Video::VideoManager::instance->rendererManager()->startedDecoding(
                    Utils::toPlatformString(id),
                    width,
                    height);
                auto callId2 = Utils::toPlatformString(id);
                incomingVideoMuted(callId2, false);
            }));
        }),
        DRing::exportable_callback<DRing::VideoSignal::DecodingStopped>
        ([&](const std::string &id, const std::string &shmPath, bool isMixer) {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                Video::VideoManager::instance->rendererManager()->removeRenderer(Utils::toPlatformString(id));
                auto callId2 = Utils::toPlatformString(id);
                incomingVideoMuted(callId2, true);
            }));
        })
    };
    registerVideoHandlers(incomingVideoHandlers);

    using namespace Video;
    outgoingVideoHandlers =
    {
        DRing::exportable_callback<DRing::VideoSignal::DeviceAdded>
        ([this](const std::string& device) {
            MSG_("<DeviceAdded>");
        }),
        DRing::exportable_callback<DRing::VideoSignal::ParametersChanged>
        ([&](const std::string& device) {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                MSG_("<ParametersChanged>");
                auto settings = DRing::getDeviceParams(device);
                VideoManager::instance->captureManager()->activeDevice->SetDeviceProperties(
                    Utils::toPlatformString(settings["format"]),
                    stoi(settings["width"]),
                    stoi(settings["height"]),
                    stoi(settings["rate"]));
            }));
        }),
        DRing::exportable_callback<DRing::VideoSignal::StartCapture>
        ([&](const std::string& device) {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                VideoManager::instance->captureManager()->InitializeCameraAsync(false);
                VideoManager::instance->captureManager()->videoFrameCopyInvoker->Start();
            }));
        }),
        DRing::exportable_callback<DRing::VideoSignal::StopCapture>
        ([&]() {
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

    nameRegistrationHandlers =
    {
        DRing::exportable_callback<DRing::ConfigurationSignal::KnownDevicesChanged>([&](const std::string& accountId, const std::map<std::string, std::string>& devices)
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                MSG_("KnownDevicesChanged ---> C PAS FINI");
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::ExportOnRingEnded>([&](const std::string& accountId, int status, const std::string& pin)
        {
            auto accountId2 = Utils::toPlatformString(accountId);
            auto pin2 = (pin.empty()) ? "Error bad password" : "Your generated pin : " + Utils::toPlatformString(pin);
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                exportOnRingEnded(accountId2, pin2);
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::RegisteredNameFound>(
        [this](const std::string &accountId, int status, const std::string &address, const std::string &name) {
            MSG_("<RegisteredNameFound>" + name + " : " + address + " status=" +std::to_string(status));
            if (accountId.empty() && address.empty() && name.empty())
                return;
            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
            ref new DispatchedHandler([=]() {
                switch (status)
                {
                case 0: // everything went fine. Name/address pair was found.
                    registeredNameFound(LookupStatus::SUCCESS, accountId, address, name);
                    break;
                case 1: // provided name is not valid.
                    registeredNameFound(LookupStatus::INVALID_NAME, accountId, address, name);
                    break;
                case 2: // everything went fine. Name/address pair was not found.
                    registeredNameFound(LookupStatus::NOT_FOUND, accountId, address, name);
                    break;
                case 3: // An error happened
                    registeredNameFound(LookupStatus::ERRORR, accountId, address, name);
                    break;
                }
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::VolatileDetailsChanged>(
        [this](const std::string& accountId, const std::map<std::string, std::string>& details) {
            ref new DispatchedHandler([=]() {
                volatileDetailsChanged(accountId, details);
            });
        })
    };
    registerConfHandlers(nameRegistrationHandlers);

    trustRequestHandlers =
    {
        DRing::exportable_callback<DRing::ConfigurationSignal::IncomingTrustRequest>(
            [&](const std::string& account_id,
                const std::string& from,
                const std::vector<uint8_t>& payload,
                time_t received)
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                auto payloadString = std::string(payload.begin(), payload.end());
                MSG_("IncomingTrustRequest");
                MSG_("account_id = " + account_id);
                MSG_("from = " + from);
                MSG_("received = " + received.ToString());
                MSG_("payload = " + payloadString);

                // First check if this TR has a corresponding contact. If not, add a contact
                // to the account's contact list with a trust status flag indicating that
                // it should be treated as a TR, and only appear in the contact request list.
                if (auto contactListModel = AccountsViewModel::instance->getContactListModel(std::string(account_id))) {
                    auto fromP = Utils::toPlatformString(from);
                    auto contact = contactListModel->findContactByRingId(fromP);

                    // If the contact exists, we should check to see if we have previously
                    // sent a TR to the peer. If, so we can accept this TR immediately.
                    // Otherwise, if it is not already been trusted, we can ignore it completely.
                    if (contact) {
                        if (contact->_trustStatus == TrustStatus::CONTACT_REQUEST_SENT) {
                            // get the vcard first
                            auto vcard = contact->getVCard();
                            auto parsedPayload = VCardUtils::parseContactRequestPayload(payloadString);
                            vcard->setData(parsedPayload.at("VCARD"));
                            vcard->completeReception();

                            DRing::acceptTrustRequest(account_id, from);
                            MSG_("Auto accepted IncomingTrustRequest");
                            return;
                        }
                        else if (contact->_trustStatus != TrustStatus::UNKNOWN) {
                            MSG_("IncomingTrustRequest ignored");
                            return;
                        }
                    }
                    else {
                        // No contact found, so add a new contact with the INCOMNG_CONTACT_REQUEST trust status flag
                        contact = contactListModel->addNewContact("", fromP, TrustStatus::INCOMING_CONTACT_REQUEST);
                    }

                    // The visible ring id will potentially be replaced by a username after a lookup
                    RingD::instance->lookUpAddress(account_id, Utils::toPlatformString(from));

                    auto vcard = contact->getVCard();
                    auto parsedPayload = VCardUtils::parseContactRequestPayload(payloadString);
                    vcard->setData(parsedPayload.at("VCARD"));
                    vcard->completeReception();

                    // The name is the ring id for now
                    contact->_name = Utils::toPlatformString(from);
                    contact->_displayName = Utils::toPlatformString(vcard->getPart("FN"));

                    contactListModel->saveContactsToFile();
                    AccountsViewModel::instance->raiseUnreadContactRequest();

                    SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                    //SmartPanelItemsViewModel::instance->update();

                    // Add a corresponding contact request control item to the list.
                    auto newContactRequest = ref new ContactRequestItem();
                    newContactRequest->_contact = contact;
                    ContactRequestItemsViewModel::instance->itemsList->InsertAt(0, newContactRequest);

                    ContactRequestItemsViewModel::instance->refreshFilteredItemsList();
                    ContactRequestItemsViewModel::instance->update();
                }
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::ContactAdded>(
            [&](const std::string& account_id, const std::string& uri, bool confirmed)
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                MSG_("ContactAdded");
                MSG_("account_id = " + account_id);
                MSG_("uri = " + uri);
                MSG_("confirmed = " + confirmed.ToString());

                // If confirmed is false, we have just sent the TR and nothing need be done.
                // If confirmed is true, the sent TR has been accepted and we can change the
                // TrustStatus flag for the contact under the account_id, that matches the uri
                if (confirmed) {
                    if (auto contactListModel = AccountsViewModel::instance->getContactListModel(std::string(account_id))) {
                        auto contact = contactListModel->findContactByRingId(Utils::toPlatformString(uri));
                        if (contact == nullptr) {
                            contact = contactListModel->addNewContact(  Utils::toPlatformString(uri),
                                                                        Utils::toPlatformString(uri),
                                                                        TrustStatus::TRUSTED);
                            RingD::instance->lookUpAddress(account_id, Utils::toPlatformString(uri));
                        }
                        else
                            contact->_trustStatus = TrustStatus::TRUSTED;

                        SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                        //SmartPanelItemsViewModel::instance->update();

                        ContactRequestItemsViewModel::instance->update();

                        contactListModel->saveContactsToFile();
                    }
                }
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::ContactRemoved>(
            [&](const std::string& account_id, const std::string& uri, bool banned)
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                MSG_("ContactRemoved");
                MSG_("account_id = " + account_id);
                MSG_("uri = " + uri);
                MSG_("banned = " + banned.ToString());

                // It's currently not clear to me how this signal is pertinent to the UWP client
            }));
        }),
            DRing::exportable_callback<DRing::PresenceSignal::NewBuddyNotification>(
                [&](const std::string& account_id, const std::string& buddy_uri, int status, const std::string& /*line_status*/)
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                MSG_("NewBuddyNotification");
                MSG_("account_id = " + account_id);
                MSG_("uri = " + buddy_uri);
                MSG_("status = " + status);
                // react to presence
                dispatcher->RunAsync(CoreDispatcherPriority::High,
                    ref new DispatchedHandler([=]() {
                    newBuddyNotification(account_id, buddy_uri, status);
                }));
            }));
        })
    };
    registerConfHandlers(trustRequestHandlers);
}

void
RingD::init()
{
    if (daemonInitialized_) {
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
        ref new DispatchedHandler([=]() {
            finishCaptureDeviceEnumeration();
        }));
        return;
    }

    setLoadingStatusText("Starting Ring...", "#ff000000");

    gnutls_global_init();
    RingD::instance->registerCallbacks();
    RingD::instance->initDaemon( DRing::DRING_FLAG_CONSOLE_LOG | DRing::DRING_FLAG_DEBUG );
    Video::VideoManager::instance->captureManager()->EnumerateWebcamsAsync();

    daemonInitialized_ = true;
}

void
RingD::deinit()
{
    DRing::fini();
    gnutls_global_deinit();
}

void
RingD::initDaemon(int flags)
{
    DRing::init(static_cast<DRing::InitFlag>(flags));
}

void
RingD::startDaemon()
{
    if (daemonRunning_) {
        ERR_("daemon already runnging");
        return;
    }
    //eraseCacheFolder();
    editModeOn_ = true;

    IAsyncAction^ action = ThreadPool::RunAsync(ref new WorkItemHandler([=](IAsyncAction^ spAction)
    {
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
            ref new DispatchedHandler([=]() {
            if (!isInWizard) {
                setLoadingStatusText("Loading from config...", "#ff000000");
            }
        }));

        daemonRunning_ = DRing::start();

        auto vcm = Video::VideoManager::instance->captureManager();
        if (vcm->deviceList->Size > 0) {
            std::string deviceName = DRing::getDefaultDevice();
            std::map<std::string, std::string> settings = DRing::getSettings(deviceName);
            int rate = stoi(settings["rate"]);
            std::string size = settings["size"];
            std::string::size_type pos = size.find('x');
            int width = std::stoi(size.substr(0, pos));
            int height = std::stoi(size.substr(pos + 1, size.length()));
            for (auto dev : vcm->deviceList) {
                if (!Utils::toString(dev->name()).compare(deviceName))
                    vcm->activeDevice = dev;
            }
            vcm->activeDevice->SetDeviceProperties("", width, height, rate);
            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
            ref new DispatchedHandler([=]() {
                finishCaptureDeviceEnumeration();
            }));
        }

        if (!daemonRunning_) {
            ERR_("\ndaemon didn't start.\n");
            return;
        }
        else {
            switch (_startingStatus) {
            case StartingStatus::REGISTERING_ON_THIS_PC:
            case StartingStatus::REGISTERING_THIS_DEVICE:
            {
                break;
            }
            case StartingStatus::NORMAL:
            default:
            {
                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
                ref new DispatchedHandler([=]() {
                    getAccounts();
                }));
                break;
            }
            }

            /* at this point the config.yml is safe. */
            std::string tokenFile = localFolder_ + "\\creation.token";
            if (fileExists(tokenFile)) {
                fileDelete(tokenFile);
            }

            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
                ref new DispatchedHandler([=]() {
                setLoadingStatusText("Ring started successfully", "#ff00ff00");
            }));

            if (!isInWizard) {
                TimeSpan delay;
                delay.Duration = 10000000;
                ThreadPoolTimer^ delayTimer = ThreadPoolTimer::CreateTimer(
                    ref new TimerElapsedHandler([this](ThreadPoolTimer^ source)
                {
                    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                        ref new DispatchedHandler([=]() {
                        auto frame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Window::Current->Content);
                        dynamic_cast<RingClientUWP::MainPage^>(frame->Content)->showLoadingOverlay(false, false);
                    }));
                }), delay);
            }

            while (daemonRunning) {
                DRing::pollEvents();
                dequeueTasks();
                Sleep(5);
            }
        }
    },Platform::CallbackContext::Any), WorkItemPriority::High);
}

RingD::RingD()
{
    this->stateChange += ref new StateChange(this, &RingD::onStateChange);
    ringtone_ = ref new Ringtone("default.wav");
    localFolder_ = Utils::toString(ApplicationData::Current->LocalFolder->Path);
    callIdsList_ = ref new Vector<String^>();
    currentCallId = nullptr;
    WriteLine("XBOX: " + isOnXBox.ToString());
}

void
RingD::onStateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code)
{
    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
        CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
    {
        if (state == CallStatus::OUTGOING_RINGING ||
            state == CallStatus::IN_PROGRESS) {
            try {
                Configuration::UserPreferences::instance->sendVCard(Utils::toString(callId));
            }
            catch (Exception^ e) {
                EXC_(e);
            }
        }

        if (state == CallStatus::INCOMING_RINGING) {
            if (isInBackground) {
                ringtone_->Start();
                ShowCallToast(callId);
            }
            else
                ringtone_->Start();
        }

        if (state == CallStatus::IN_PROGRESS) {
            ringtone_->Stop();
        }

        if (state == CallStatus::ENDED ||
            (state == CallStatus::NONE && code == 106)) {
            DRing::hangUp(Utils::toString(callId));
            ringtone_->Stop();
        }
    }));
}

std::string
RingD::getLocalFolder()
{
    return localFolder_ + Utils::toString("\\");
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
        case Request::PlaceCall:
        {
            auto selectedAccount = AccountListItemsViewModel::instance->_selectedItem->_account;
            std::string callId;
            if (selectedAccount->accountType_ == "RING")
                callId = DRing::placeCall(task->_accountId_new, "ring:" + task->_ringId_new);
            else if (selectedAccount->accountType_ == "SIP")
                callId = DRing::placeCall(task->_accountId_new, "sip:" + Utils::toString(task->_sipUsername));
            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                if (auto contactListModel = AccountsViewModel::instance->getContactListModel(task->_accountId_new)) {
                    Contact^ contact;
                    if (selectedAccount->accountType_ == "RING")
                        contact = contactListModel->findContactByRingId(Utils::toPlatformString(task->_ringId_new));
                    else if (selectedAccount->accountType_ == "SIP")
                        contact = contactListModel->findContactByName(task->_sipUsername);
                    if (auto item = SmartPanelItemsViewModel::instance->findItem(contact)) {
                        item->_callId = Utils::toPlatformString(callId);
                        if (!callId.empty())
                            callPlaced(Utils::toPlatformString(callId));
                    }
                }
            }));
        }
        break;
        case Request::AddRingAccount:
        {
            std::map<std::string, std::string> ringAccountDetails;
            ringAccountDetails.insert(std::make_pair(DRing::Account::ConfProperties::ALIAS
                                      , Utils::toString(task->_alias)));
            ringAccountDetails.insert(std::make_pair(DRing::Account::ConfProperties::ARCHIVE_PASSWORD,
                                      Utils::toString(task->_password)));
            ringAccountDetails.insert(std::make_pair(DRing::Account::ConfProperties::TYPE,"RING"));
            ringAccountDetails.insert(std::make_pair(DRing::Account::ConfProperties::UPNP_ENABLED
                                      , (task->_upnp)? ring::TRUE_STR : ring::FALSE_STR));
            ringAccountDetails.insert(std::make_pair(DRing::Account::VolatileProperties::REGISTERED_NAME
                                      , Utils::toString(task->_registeredName)));


            auto newAccountId = DRing::addAccount(ringAccountDetails);

            if (!task->_registeredName->IsEmpty())
                registerName_new(newAccountId, Utils::toString(task->_password), Utils::toString(task->_registeredName));
        }
        break;
        case Request::AddSIPAccount:
        {
            std::map<std::string, std::string> sipAccountDetails;
            sipAccountDetails.insert(std::make_pair(DRing::Account::ConfProperties::ALIAS
                                                    , Utils::toString(task->_alias)));
            sipAccountDetails.insert(std::make_pair(DRing::Account::ConfProperties::TYPE,"SIP"));
            sipAccountDetails.insert(std::make_pair(DRing::Account::ConfProperties::PASSWORD
                                                    , Utils::toString(task->_sipPassword)));
            sipAccountDetails.insert(std::make_pair(DRing::Account::ConfProperties::HOSTNAME
                                                    , Utils::toString(task->_sipHostname)));
            sipAccountDetails.insert(std::make_pair(DRing::Account::ConfProperties::USERNAME
                                                    , Utils::toString(task->_sipUsername)));
            DRing::addAccount(sipAccountDetails);
        }
        break;
        case Request::RefuseIncommingCall:
        {
            auto callId = task->_callId;
            auto callId2 = Utils::toString(callId);
            DRing::refuse(callId2);
        }
        break;
        case Request::AcceptIncommingCall:
        {
            auto callId = task->_callId;
            auto callId2 = Utils::toString(callId);
            DRing::accept(callId2);
        }
        break;
        case Request::CancelOutGoingCall:
        case Request::HangUpCall:
        {
            auto callId = task->_callId;

            DRing::hangUp(Utils::toString(callId));

        }
        break;
        case Request::PauseCall:
        {
            DRing::hold(task->_callid_new);
        }
        break;
        case Request::UnPauseCall:
        {
            DRing::unhold(task->_callid_new);
        }
        break;
        case Request::RegisterDevice:
        {
            auto pin = Utils::toString(task->_pin);
            auto password = Utils::toString(task->_password);

            std::map<std::string, std::string> deviceDetails;
            deviceDetails.insert(std::make_pair(DRing::Account::ConfProperties::TYPE, "RING"));
            deviceDetails.insert(std::make_pair(DRing::Account::ConfProperties::ARCHIVE_PIN, pin));
            deviceDetails.insert(std::make_pair(DRing::Account::ConfProperties::ARCHIVE_PASSWORD, password));

            DRing::addAccount(deviceDetails);
        }
        break;
        case Request::GetKnownDevices:
        {
            auto accountId = task->_accountId;
            auto accountId2 = Utils::toString(accountId);

            auto devicesList = DRing::getKnownRingDevices(accountId2);
            if (devicesList.empty())
                break;

            auto devicesList2 = translateKnownRingDevices(devicesList);

            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
            ref new DispatchedHandler([=]() {
                devicesListRefreshed(devicesList2);
            }));
            break;
        }
        case Request::ExportOnRing:
        {
            auto accountId = task->_accountId;
            auto password = task->_password;

            auto accountId2 = Utils::toString(accountId);
            auto password2 = Utils::toString(password);

            DRing::exportOnRing(accountId2, password2);
            break;
        }
        case Request::UpdateAccount:
        {
            auto account = AccountListItemsViewModel::instance->findItem(Utils::toPlatformString(task->_accountId_new))->_account;
            std::map<std::string, std::string> accountDetails = DRing::getAccountDetails(task->_accountId_new);
            std::map<std::string, std::string> accountDetailsOld(accountDetails);

            accountDetails[DRing::Account::ConfProperties::ALIAS] = Utils::toString(account->name_);
            accountDetails[DRing::Account::ConfProperties::ENABLED] = (account->_active) ? ring::TRUE_STR : ring::FALSE_STR;

            if (accountDetails[DRing::Account::ConfProperties::TYPE] == "RING") {
                accountDetails[DRing::Account::ConfProperties::UPNP_ENABLED] = (account->_upnpState) ? ring::TRUE_STR : ring::FALSE_STR;
                accountDetails[DRing::Account::ConfProperties::AUTOANSWER] = (account->_autoAnswer) ? ring::TRUE_STR : ring::FALSE_STR;
                accountDetails[DRing::Account::ConfProperties::DHT::PUBLIC_IN_CALLS] = (account->_dhtPublicInCalls) ? ring::TRUE_STR : ring::FALSE_STR;
                if (accountDetails == accountDetailsOld)
                    break;
                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    setLoadingStatusText("Updating account...", "#ff00ff00");
                    auto frame = dynamic_cast<Frame^>(Window::Current->Content);
                    dynamic_cast<RingClientUWP::MainPage^>(frame->Content)->showLoadingOverlay(true, true);
                }));
            }
            else {
                accountDetails[DRing::Account::ConfProperties::HOSTNAME] = Utils::toString(account->_sipHostname);
                accountDetails[DRing::Account::ConfProperties::PASSWORD] = Utils::toString(account->_sipPassword);
                accountDetails[DRing::Account::ConfProperties::USERNAME] = Utils::toString(account->_sipUsername);
                if (accountDetails == accountDetailsOld)
                    break;
            }

            DRing::setAccountDetails(Utils::toString(account->accountID_), accountDetails);
            break;
        }
        case Request::DeleteAccount:
        {
            auto accountId = task->_accountId;
            auto accountId2 = Utils::toString(accountId);

            std::map<std::string, std::string> accountDetails = DRing::getAccountDetails(task->_accountId_new);

            if (accountDetails[DRing::Account::ConfProperties::TYPE] == "RING")
                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                setLoadingStatusText("Deleting account...", "#ffff0000");
                auto frame = dynamic_cast<Frame^>(Window::Current->Content);
                dynamic_cast<RingClientUWP::MainPage^>(frame->Content)->showLoadingOverlay(true, true);
            }));

            DRing::removeAccount(accountId2);
            break;
        }
        case Request::GetCallsList:
        {
            auto callsList = DRing::getCallList();
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([&]() {
                callsListRecieved(callsList);
            }));
            MSG_("list of calls returned by the daemon :");
            for (auto call : callsList)
                MSG_(call);
            MSG_("[EOL]"); // end of list
            break;
        }
        case Request::KillCall:
        {
            auto callId = task->_callId;
            auto callId2 = Utils::toString(callId);
            MSG_("asking daemon to kill : " + callId2);
            DRing::hangUp(callId2);
            break;
        }
        case Request::switchDebug:
        {
            debugModeOn_ = !debugModeOn_;
            break;
        }
        case Request::MuteVideo:
        {
            auto callId = Utils::toString(task->_callId);
            bool muted = task->_muted;
            DRing::muteLocalMedia(callId, DRing::Media::Details::MEDIA_TYPE_VIDEO, muted);
        }
        case Request::MuteAudio:
        {
            DRing::muteLocalMedia(task->_callid_new
                                  , DRing::Media::Details::MEDIA_TYPE_AUDIO
                                  , task->_audioMuted_new);
        }
        case Request::LookUpName:
        {
            auto alias = task->_alias;
            DRing::lookupName(task->_accountId_new, "", Utils::toString(alias));
            break;
        }
        case Request::LookUpAddress:
        {
            DRing::lookupAddress(task->_accountId_new, "", Utils::toString(task->_address));
            break;
        }
        case Request::RegisterName:
        {
            auto accountDetails = DRing::getAccountDetails(task->_accountId_new);
            bool result;

            if (accountDetails[DRing::Account::ConfProperties::USERNAME].empty())
                registerName_new(task->_accountId_new, task->_password_new, task->_publicUsername_new);
            else
            {
                result = DRing::registerName(task->_accountId_new, task->_password_new, task->_publicUsername_new);

                CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {
                    nameRegistred(result);
                }));
            }

            break;
        }
        case Request::SendContactRequest:
        {
            std::vector<uint8_t> payload(task->_payload.begin(), task->_payload.end());
            DRing::sendTrustRequest(task->_accountId_new, task->_ringId_new, payload);
            MSG_("Sent Trust Request");
            break;
        }
        case Request::RemoveContact:
        {
            DRing::removeContact(task->_accountId_new, task->_ringId_new);
            MSG_("Removed contact (daemon)");
            break;
        }
        default:
            break;
        }
        tasksList_.pop();
    }
}

void RingClientUWP::RingD::getCallsList()
{
    auto task = ref new RingD::Task(Request::GetCallsList);

    tasksList_.push(task);

}

void RingClientUWP::RingD::killCall(String ^ callId)
{
    if (callId == "-all") {
        auto callsList = DRing::getCallList();

        for (auto call : callsList) {
            auto task = ref new RingD::Task(Request::KillCall);
            task->_callId = Utils::toPlatformString(call);

            tasksList_.push(task);
        }
        return;
    }

    auto task = ref new RingD::Task(Request::KillCall);
    task->_callId = callId;

    tasksList_.push(task);

}

void RingClientUWP::RingD::switchDebug()
{
    auto task = ref new RingD::Task(Request::switchDebug);

    tasksList_.push(task);
}

void RingClientUWP::RingD::muteVideo(String ^ callId, bool muted)
{
    auto task = ref new RingD::Task(Request::MuteVideo);

    task->_callId = callId;
    task->_muted = muted;

    tasksList_.push(task);
}

void RingClientUWP::RingD::muteAudio(const std::string& callId, bool muted)
{
    auto task = ref new RingD::Task(Request::MuteAudio);

    task->_callid_new = callId;
    task->_audioMuted_new = muted;

    tasksList_.push(task);
}

void
RingD::registerName(String^ accountId, String^ password, String^ username)
{
    auto task = ref new RingD::Task(Request::RegisterName);
    task->_accountId = ref new String(accountId->Data());
    task->_password = password;
    task->_alias = username;

    tasksList_.push(task);
}

void RingClientUWP::RingD::registerName_new(const std::string & accountId, const std::string & password, const std::string & username)
{
    auto task = ref new RingD::Task(Request::RegisterName);
    task->_accountId_new = accountId;
    task->_password_new = password;
    task->_publicUsername_new = username;

    tasksList_.push(task);
}

void
RingD::removeContact(const std::string& accountId, const std::string& uri)
{
    auto task = ref new RingD::Task(Request::RemoveContact);
    task->_accountId_new = accountId;
    task->_ringId_new = uri;
    tasksList_.push(task);
}

void
RingD::sendContactRequest(const std::string& accountId, const std::string& uri, const std::string& payload)
{
    auto task = ref new RingD::Task(Request::SendContactRequest);
    task->_accountId_new = accountId;
    task->_ringId_new = uri;
    task->_payload = payload;
    tasksList_.push(task);
}

std::map<std::string, std::string>
RingClientUWP::RingD::getVolatileAccountDetails(Account^ account)
{
    return DRing::getVolatileAccountDetails(Utils::toString(account->accountID_));
}

void
RingD::lookUpName(const std::string& accountId, String ^ name)
{
    auto task = ref new RingD::Task(Request::LookUpName);
    task->_alias = name;
    task->_accountId_new = accountId;

    tasksList_.push(task);
}

void
RingD::lookUpAddress(const std::string& accountId, String^ address)
{
    auto task = ref new RingD::Task(Request::LookUpAddress);
    task->_address = address;
    task->_accountId_new = accountId;

    tasksList_.push(task);
}

std::string RingClientUWP::RingD::registeredName(Account^ account)
{
    auto volatileAccountDetails = DRing::getVolatileAccountDetails(Utils::toString(account->accountID_));
    return volatileAccountDetails[DRing::Account::VolatileProperties::REGISTERED_NAME];
}

RingClientUWP::CallStatus RingClientUWP::RingD::translateCallStatus(String^ state)
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

    if (state == "HOLD")
        return CallStatus::PAUSED;

    if (state == "PEER_PAUSED")
        return CallStatus::PEER_PAUSED;

    return CallStatus::NONE;
}

String^
RingD::getUserName()
{
    auto users = User::FindAllAsync();
    return nullptr;
}

Vector<String^>^ RingClientUWP::RingD::translateKnownRingDevices(const std::map<std::string, std::string> devices)
{
    auto devicesList = ref new Vector<String^>();

    for (auto i : devices) {
        MSG_("devices.first = " + i.first);
        MSG_("devices.second = " + i.second);
        auto deviceName = Utils::toPlatformString(i.second);
        devicesList->Append(deviceName);
    }

    return devicesList;
}

void RingClientUWP::RingD::setFullScreenMode()
{
    if (ApplicationView::GetForCurrentView()->TryEnterFullScreenMode()) {
        MSG_("TryEnterFullScreenMode succeeded");
        fullScreenToggled(true);
    }
    else {
        ERR_("TryEnterFullScreenMode failed");
    }
}

void RingClientUWP::RingD::setWindowedMode()
{
    ApplicationView::GetForCurrentView()->ExitFullScreenMode();
    MSG_("ExitFullScreenMode");
    fullScreenToggled(false);
}

void RingClientUWP::RingD::toggleFullScreen()
{
    if (isFullScreen)
        setWindowedMode();
    else
        setFullScreenMode();
}

void
RingD::raiseMessageDataLoaded()
{
    messageDataLoaded();
}