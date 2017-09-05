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

#include "RingD.h"

#include "RingDebug.h"
#include "NetUtils.h"
#include "FileUtils.h"
#include "UserPreferences.h"
#include "AccountItemsViewModel.h"
#include "Video.h"
#include "ResourceManager.h"

#include "dring.h"
#include "dring/call_const.h"
#include "callmanager_interface.h"
#include "configurationmanager_interface.h"
#include "presencemanager_interface.h"
#include "videomanager_interface.h"
#include "account_const.h"
#include "string_utils.h"
#include "gnutls\gnutls.h"
#include "media_const.h"

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
RingD::registerCallbacks()
{
    dispatcher = CoreApplication::MainView->CoreWindow->Dispatcher;

    using namespace Utils;
    using namespace DRing;

    callHandlers = {
        exportable_callback<CallSignal::IncomingCall>(
            [this](const std::string& accountId, const std::string& callId, const std::string& from) {
                MSG_("SIGNAL<IncomingCall> " + ("(accountId = " + accountId + ", callId = " + callId + ", from = " + from + ")"));
                Threading::runOnUIThread([this, accountId, callId, from]() {
                    incomingCall(toPlatformString(accountId), toPlatformString(callId), toPlatformString(from));
                });
            }),
        DRing::exportable_callback<DRing::CallSignal::StateChange>(
            [this](const std::string& callId, const std::string& state, int code) {
                MSG_("SIGNAL<StateChange> " + ("(callId = " + callId + ", state = " + state + ", code = " + std::to_string(code) + ")"));
                Threading::runOnUIThread([this, callId, state, code]() {
                    stateChange(toPlatformString(callId), translateCallStatus(toPlatformString(state)), code);
                });
            }),
        DRing::exportable_callback<DRing::CallSignal::SmartInfo>([this](
                    const std::map<std::string,
                    std::string>& info)
        {
            Utils::Threading::runOnUIThread([this, info]() {updateSmartInfo(info); });
        }),
        DRing::exportable_callback<DRing::CallSignal::PeerHold>([this](
                    const std::string& callId,
                    bool state)
        {
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
        DRing::exportable_callback<DRing::CallSignal::IncomingMessage>([this](
                    const std::string& callId,
                    const std::string& from,
                    const std::map<std::string,
                    std::string>& payloads)
        {
            MSG_("<IncomingMessage>");
            MSG_("callId = " + callId);
            MSG_("from = " + from);

            handleIncomingMessage(callId, "", from, payloads);
        }),
        DRing::exportable_callback<DRing::DebugSignal::MessageSend>([this](const std::string& msg)
        {
            if (debugModeOn_) {
                DMSG_(msg);
            }
        })
    };
    registerCallHandlers(callHandlers);

    configurationHandlers = {
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
        DRing::exportable_callback<DRing::ConfigurationSignal::AccountMessageStatusChanged>([this](
                    const std::string& account_id,
                    uint64_t message_id,
                    const std::string& to,
                    int state)
        {
            MSG_("<AccountMessageStatusChanged>");
            MSG_("account_id = " + account_id);
            MSG_("message_id = " + message_id);
            MSG_("to = " + Utils::toPlatformString(to));
            MSG_("state = " + state.ToString());
            dispatcher->RunAsync(CoreDispatcherPriority::High,
                ref new DispatchedHandler([=]() {

                // acquittement de message
                auto contactListModel = AccountsViewModel::instance->getContactList(std::string(account_id));
                if (auto contact = contactListModel->findContactByRingId(Utils::toPlatformString(to))) {
                    auto conversation = contact->_conversation;
                    if (conversation) {
                        for (const auto& msg : conversation->_messages) {
                            if (msg->MessageIdInteger == message_id &&
                                state == Utils::toUnderlyingValue(DRing::Account::MessageStates::SENT)) {
                                messageStatusUpdated(msg->MessageId, state);
                                msg->IsReceived = true;
                                contact->saveConversationToFile();
                            }
                        }
                    }
                }
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::RegistrationStateChanged>([this](
                    const std::string& account_id, const std::string& state,
                    int detailsCode, const std::string& detailsStr)
        {
            MSG_("<RegistrationStateChanged>: ID = " + account_id + " state = " + state);
            auto allAccountDetails = getAllAccountDetails();
            Utils::Threading::runOnUIThread([this, state, account_id, allAccountDetails = std::move(allAccountDetails)]() {
                parseAccountDetails(allAccountDetails);
                if (state == DRing::Account::States::REGISTERED) {
                    if (auto accountItem = AccountItemsViewModel::instance->findItem(Utils::toPlatformString(account_id)))
                        accountItem->_registrationState = RegistrationState::REGISTERED;
                    registrationStateRegistered(account_id);
                }
                else if (state == DRing::Account::States::UNREGISTERED) {
                    if (auto accountItem = AccountItemsViewModel::instance->findItem(Utils::toPlatformString(account_id)))
                        accountItem->_registrationState = RegistrationState::UNREGISTERED;
                    registrationStateUnregistered(account_id);
                }
                else if (state == DRing::Account::States::TRYING) {
                    if (auto accountItem = AccountItemsViewModel::instance->findItem(Utils::toPlatformString(account_id)))
                        accountItem->_registrationState = RegistrationState::TRYING;
                    registrationStateTrying(account_id);
                }
                else {
                    registrationStateErrorGeneric(account_id);
                }
                if (isUpdatingAccount) {
                    onAccountUpdated();
                }
            });
            if (state == DRing::Account::States::REGISTERED) {
                subscribeBuddies();
            }
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::AccountsChanged>([this]()
        {
            MSG_("<AccountsChanged>");
            if (isDeletingAccount) {
                OnaccountDeleted();
            }
            else {
                auto allAccountDetails = getAllAccountDetails();
                Utils::Threading::runOnUIThread([this, allAccountDetails = std::move(allAccountDetails)]() {
                    parseAccountDetails(allAccountDetails);
                });
            }
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::NameRegistrationEnded>([this](
                    const std::string& account_id, int state, const std::string& name)
        {
            MSG_("<NameRegistrationEnded>");
            bool res = state == 0;
            Utils::Threading::runOnUIThread([this, res, account_id]() {
                nameRegistered(res, Utils::toPlatformString(account_id));
            });
            if (!res)
                return;

            // old
            auto account = AccountsViewModel::instance->findItem(Utils::toPlatformString(account_id));
            account->_username = Utils::toPlatformString(name);

            // new
            auto accountItem = AccountItemsViewModel::instance->findItem(Utils::toPlatformString(account_id));
            accountItem->_registeredName = Utils::toPlatformString(name);

        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::GetAppDataPath>([this](
                    const std::string& name, std::vector<std::string>* paths)
        {
            paths->emplace_back(localFolder_);
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::GetDeviceName>([this](
                    std::vector<std::string>* hostNames)
        {
            hostNames->emplace_back(Utils::getHostName());
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::KnownDevicesChanged>([&](
                    const std::string& accountId, const std::map<std::string, std::string>& devices)
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                if (isAddingDevice) {
                    isAddingDevice = false;
                    hideLoadingOverlay("This device has been successfully added", SuccessColor);
                }
                MSG_("<KnownDevicesChanged>");
                getKnownDevices(Utils::toPlatformString(accountId));
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::ExportOnRingEnded>([&](
                    const std::string& accountId, int status, const std::string& pin)
        {
            auto accountId2 = Utils::toPlatformString(accountId);
            auto pin2 = (pin.empty()) ? "Error bad password" : "Your generated PIN : " + Utils::toPlatformString(pin);
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                exportOnRingEnded(accountId2, pin2);
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::RegisteredNameFound>([this](
                    const std::string &accountId, int status, const std::string &address, const std::string &name)
        {
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
        DRing::exportable_callback<DRing::ConfigurationSignal::VolatileDetailsChanged>([this](
                    const std::string& accountId, const std::map<std::string, std::string>& details)
        {
            ref new DispatchedHandler([=]() {
                volatileDetailsChanged(accountId, details);
            });
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::IncomingTrustRequest>([&](
                    const std::string& account_id,
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
                if (auto contactListModel = AccountsViewModel::instance->getContactList(std::string(account_id))) {
                    auto fromP = Utils::toPlatformString(from);
                    auto contact = contactListModel->findContactByRingId(fromP);

                    // If the contact exists, we should check to see if we have previously
                    // sent a TR to the peer. If, so we can accept this TR immediately.
                    // Otherwise, if it is not already been trusted, we can ignore it completely.
                    if (contact) {
                        if (contact->_trustStatus == TrustStatus::CONTACT_REQUEST_SENT) {
                            // get the vcard first
                            auto vcard = contact->getVCard();
                            auto parsedPayload = profile::parseContactRequestPayload(payloadString);
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
                        contact = contactListModel->addNewContact("", fromP, TrustStatus::INCOMING_CONTACT_REQUEST, false);
                    }

                    // The visible ring id will potentially be replaced by a username after a lookup
                    RingD::instance->lookUpAddress(account_id, Utils::toPlatformString(from));

                    auto vcard = contact->getVCard();
                    auto parsedPayload = profile::parseContactRequestPayload(payloadString);
                    vcard->setData(parsedPayload.at("VCARD"));
                    vcard->completeReception();

                    // The name is the ring id for now
                    contact->_name = Utils::toPlatformString(from);
                    contact->_displayName = Utils::toPlatformString(vcard->getPart("FN"));

                    contactListModel->saveContactsToFile();
                    AccountsViewModel::instance->raiseUnreadContactRequest();

                    SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                    SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);

                    // Add a corresponding contact request control item to the list.
                    auto newContactRequest = ref new ContactRequestItem();
                    newContactRequest->_contact = contact;
                    ContactRequestItemsViewModel::instance->itemsList->InsertAt(0, newContactRequest);

                    ContactRequestItemsViewModel::instance->refreshFilteredItemsList();
                    ContactRequestItemsViewModel::instance->update(ViewModel::NotifyStrings::notifyContactRequestItem);
                }
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::ContactAdded>([&](
                    const std::string& account_id, const std::string& uri, bool confirmed)
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
                    if (auto contactListModel = AccountsViewModel::instance->getContactList(std::string(account_id))) {
                        auto contact = contactListModel->findContactByRingId(Utils::toPlatformString(uri));
                        if (contact == nullptr) {
                            contact = contactListModel->addNewContact(  Utils::toPlatformString(uri),
                                                                        Utils::toPlatformString(uri),
                                                                        TrustStatus::TRUSTED, false);
                            RingD::instance->lookUpAddress(account_id, Utils::toPlatformString(uri));
                        }
                        else {
                            contact->_trustStatus = TrustStatus::TRUSTED;
                        }

                        SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                        SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);
                        ContactRequestItemsViewModel::instance->update(ViewModel::NotifyStrings::notifyContactRequestItem);

                        AccountsViewModel::instance->raiseContactDataModified(Utils::toPlatformString(uri), contact);

                        contactListModel->saveContactsToFile();
                    }
                }
            }));
        }),
        DRing::exportable_callback<DRing::ConfigurationSignal::ContactRemoved>([&](
                    const std::string& account_id, const std::string& uri, bool banned)
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
        DRing::exportable_callback<DRing::ConfigurationSignal::DeviceRevocationEnded>([&](
                    const std::string& accountId, const std::string& device, int status)
        {
            MSG_("<DeviceRevocationEnded>");
            MSG_("account_id = " + accountId);
            MSG_("device = " + device);
            MSG_("status = " + status.ToString());
            switch (Utils::toEnum<DeviceRevocationResult>(status)) {
            case DeviceRevocationResult::INVALID_CERTIFICATE:
                hideLoadingOverlay("Certificate error while revoking device ID: " + Utils::toPlatformString(device), ErrorColor);
                break;
            case DeviceRevocationResult::INVALID_PASSWORD:
                hideLoadingOverlay("Incorrect account password. Can't revoke device ID: " + Utils::toPlatformString(device), ErrorColor);
                break;
            case DeviceRevocationResult::SUCCESS:
                getKnownDevices(Utils::toPlatformString(accountId));
                hideLoadingOverlay("Device with ID: " + Utils::toPlatformString(device) + " has been successfully revoked", SuccessColor);
                break;
            }
        })/*,
        DRing::exportable_callback<DRing::ConfigurationSignal::GetAppUserName>([this](
                    std::vector<std::string>* unames)
        {
            unames->emplace_back(Utils::toString(
                UserModel::instance->firstName +
                "." +
                UserModel::instance->lastName));
        })*/
    };
    registerConfHandlers(configurationHandlers);

    using namespace Video;
    videoHandlers = {
        DRing::exportable_callback<DRing::VideoSignal::DeviceEvent>([this]()
        {
            MSG_("<DeviceEvent>");
        }),
        DRing::exportable_callback<DRing::VideoSignal::DecodingStarted>([&](
                    const std::string &id, const std::string &shmPath, int width, int height, bool isMixer)
        {
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
        DRing::exportable_callback<DRing::VideoSignal::DecodingStopped>([&](
                    const std::string &id, const std::string &shmPath, bool isMixer)
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                Video::VideoManager::instance->rendererManager()->removeRenderer(Utils::toPlatformString(id));
                auto callId2 = Utils::toPlatformString(id);
                incomingVideoMuted(callId2, true);
            }));
        }),
            DRing::exportable_callback<DRing::VideoSignal::DeviceAdded>([this](
                        const std::string& device)
        {
            MSG_("<DeviceAdded>");
        }),
        DRing::exportable_callback<DRing::VideoSignal::ParametersChanged>([&](
                    const std::string& device)
        {
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
        DRing::exportable_callback<DRing::VideoSignal::StartCapture>([&](
                    const std::string& device)
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                VideoManager::instance->captureManager()->InitializeCameraAsync(false);
                VideoManager::instance->captureManager()->videoFrameCopyInvoker->Start();
            }));
        }),
        DRing::exportable_callback<DRing::VideoSignal::StopCapture>([&]()
        {
            dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]() {
                VideoManager::instance->captureManager()->StopPreviewAsync();
                if (VideoManager::instance->captureManager()->captureTaskTokenSource)
                    VideoManager::instance->captureManager()->captureTaskTokenSource->cancel();
                VideoManager::instance->captureManager()->videoFrameCopyInvoker->Stop();
            }));
        })
    };
    registerVideoHandlers(videoHandlers);

    presenceHandlers = {
        DRing::exportable_callback<DRing::PresenceSignal::NewBuddyNotification>([&](
                    const std::string& account_id, const std::string& buddy_uri, int status, const std::string& /*line_status*/)
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
    registerPresHandlers(presenceHandlers);
}
void
RingD::onIncomingCall(String^ accountId, String^ callId, String^ from)
{
    from = Utils::TrimRingId(from);

    auto accountItem = AccountItemsViewModel::instance->findItem(accountId);
    if (auto contactListModel = AccountsViewModel::instance->getContactList(Utils::toString(accountId))) {
        Contact^ contact;
        if (accountItem->_accountType == "RING")
            contact = contactListModel->findContactByRingId(from);
        else if (accountItem->_accountType == "SIP")
            contact = contactListModel->findContactByName(from);
        if (contact) {
            auto item = SmartPanelItemsViewModel::instance->findItem(contact);
            if (item) {
                item->_callId = callId;
                if (accountItem->_autoAnswerEnabled) {
                    item->_callStatus = CallStatus::AUTO_ANSWERING;
                    SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                    stateChange(callId, CallStatus::AUTO_ANSWERING, 0);
                    return;
                }
            }
        }
    }
    stateChange(callId, CallStatus::INCOMING_RINGING, 0);
}

void
RingD::onStateChange(String ^callId, CallStatus state, int code)
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
            ringtone_->Start();
        }

        if (state == CallStatus::IN_PROGRESS) {
            ringtone_->Stop();
            if (callToastPopped_) {
                HideToast(toastCall_);
                callToastPopped_ = false;
            }
        }

        if (state == CallStatus::ENDED ||
            (state == CallStatus::NONE && code == 106)) {
            if (callToastPopped_) {
                HideToast(toastCall_);
                callToastPopped_ = false;
            }
            DRing::hangUp(Utils::toString(callId));
            ringtone_->Stop();
        }
    }));
}