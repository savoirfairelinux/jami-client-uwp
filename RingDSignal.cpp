/**************************************************************************
* Copyright (C) 2017 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
* Author: Traczyk Andreas <andreas.traczyk@savoirfairelinux.com>          *
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

/* signals (events) */
void
RingD::registerCallbacks()
{
    dispatcher = CoreApplication::MainView->CoreWindow->Dispatcher;

    registerCallHandlers();
    registerConfigurationHandlers();
    registerPresenceHandlers();
    registerVideoHandlers();
}

void
RingD::registerCallHandlers()
{
    using namespace Utils;
    using namespace DRing;

    callHandlers = {
        exportable_callback<CallSignal::IncomingCall>(
            [this](const std::string& accountId, const std::string& callId, const std::string& from) {
                MSG_SIG_("SIGNAL<IncomingCall> " + ("(accountId = " + accountId + ", callId = " + callId + ", from = " + from + ")"));
                Threading::runOnUIThread([this, accountId, callId, from]() {
                    incomingCall(toPlatformString(accountId), toPlatformString(callId), toPlatformString(from));
                });
            }),
        DRing::exportable_callback<DRing::CallSignal::StateChange>(
            [this](const std::string& callId, const std::string& state, int code) {
                MSG_SIG_("SIGNAL<StateChange> " + ("(callId = " + callId + ", state = " + state + ", code = " + std::to_string(code) + ")"));
                Threading::runOnUIThread([this, callId, state, code]() {
                    stateChange(toPlatformString(callId), translateCallStatus(toPlatformString(state)), code);
                });
            }),
        DRing::exportable_callback<DRing::CallSignal::SmartInfo>(
            [this](const std::map<std::string, std::string>& info) {
                MSG_SIG_("SIGNAL<SmartInfo>");
                Utils::Threading::runOnUIThread([this, info]() {
                    smartInfo(convertMap(info));
                });
            }),
        DRing::exportable_callback<DRing::CallSignal::PeerHold>(
            [this](const std::string& callId, bool state) {
                MSG_SIG_("SIGNAL<PeerHold> " + ("(callId = " + callId + ", state = " + toString(state.ToString()) + ")"));
                Utils::Threading::runOnUIThread([this, callId, state]() {
                    stateChange(toPlatformString(callId), (state ? CallStatus::PEER_PAUSED : CallStatus::IN_PROGRESS), 0);
                });
            }),
        DRing::exportable_callback<DRing::CallSignal::AudioMuted>(
            [this](const std::string& callId, bool state) {
                MSG_SIG_("SIGNAL<AudioMuted> " + ("(callId = " + callId + ", state = " + toString(state.ToString()) + ")"));
                Utils::Threading::runOnUIThread([this, callId, state]() {
                    audioMuted(toPlatformString(callId), state);
                });
        }),
        DRing::exportable_callback<DRing::CallSignal::VideoMuted>(
            [this](const std::string& callId, bool state) {
                MSG_SIG_("SIGNAL<VideoMuted> " + ("(callId = " + callId + ", state = " + toString(state.ToString()) + ")"));
                Utils::Threading::runOnUIThread([this, callId, state]() {
                    videoMuted(toPlatformString(callId), state);
                });
            }),
        DRing::exportable_callback<DRing::CallSignal::IncomingMessage>(
            [this](const std::string& callId, const std::string& from, const std::map<std::string, std::string>& payloads) {
                MSG_SIG_("SIGNAL<IncomingMessage> " + ("(callId = " + callId + ", from = " + from + ")"));
                Utils::Threading::runOnUIThread([this, callId, from, payloads]() {
                    incomingMessage(toPlatformString(callId), toPlatformString(from), convertMap(payloads));
                });
            }),
        DRing::exportable_callback<DRing::DebugSignal::MessageSend>(
            [this](const std::string& msg) {
                if (debugModeOn_) {
                    DMSG_(msg);
                }
            })
    };
    DRing::registerCallHandlers(callHandlers);
}

void
RingD::registerConfigurationHandlers()
{
    configurationHandlers = {
        DRing::exportable_callback<DRing::ConfigurationSignal::IncomingAccountMessage>(
            [this](const std::string& accountId, const std::string& from, const std::map<std::string, std::string>& payloads) {
                MSG_SIG_("SIGNAL<IncomingAccountMessage> " + ("(accountId = " + accountId + ", from = " + from + ")"));
                Utils::Threading::runOnUIThread([this, accountId, from, payloads]() {
                    incomingAccountMessage(toPlatformString(accountId), toPlatformString(from), convertMap(payloads));
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::AccountMessageStatusChanged>(
            [this](const std::string& accountId, uint64_t messageId, const std::string& to, int state) {
                MSG_SIG_("SIGNAL<AccountMessageStatusChanged> " + ("(accountId = " + accountId + ", messageId = " + toString(messageId.ToString()) + ", to = " + to + ", state = " + toString(state.ToString()) + ")"));
                Utils::Threading::runOnUIThread([this, accountId, messageId, to, state]() {
                    accountMessageStatusChanged(toPlatformString(accountId), messageId, toPlatformString(to), state);
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::RegistrationStateChanged>(
            [this](const std::string& accountId, const std::string& state, int detailsCode, const std::string& detailsStr) {
                MSG_SIG_("SIGNAL<RegistrationStateChanged> " + ("(accountId = " + accountId + ", state = " + state + ", detailsStr = " + detailsStr + ")"));
                Utils::Threading::runOnUIThread([this, accountId, state, detailsCode, detailsStr]() {
                    registrationStateChanged(toPlatformString(accountId), toPlatformString(state), detailsCode, toPlatformString(detailsStr));
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::AccountsChanged>(
            [this]() {
                MSG_SIG_("SIGNAL<AccountsChanged>");
                Utils::Threading::runOnUIThread([this]() {
                    accountsChanged();
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::NameRegistrationEnded>(
            [this](const std::string& accountId, int state, const std::string& name) {
                MSG_SIG_("SIGNAL<NameRegistrationEnded> " + ("(accountId = " + accountId + ", state = " + toString(state.ToString()) + ", name = " + name + ")"));
                Utils::Threading::runOnUIThread([this, accountId, state, name]() {
                    nameRegistrationEnded(toPlatformString(accountId), state, toPlatformString(name));
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::GetAppDataPath>(
            [this](const std::string& name, std::vector<std::string>* paths) {
                //MSG_SIG_("SIGNAL<GetAppDataPath> " + ("(name = " + name")"));
                paths->emplace_back(localFolder_);
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::GetDeviceName>(
            [this](std::vector<std::string>* hostNames) {
                MSG_SIG_("SIGNAL<GetDeviceName>");
                hostNames->emplace_back(Utils::getHostName());
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::KnownDevicesChanged>(
            [this](const std::string& accountId, const std::map<std::string, std::string>& devices) {
                MSG_SIG_("SIGNAL<KnownDevicesChanged> " + ("(accountId = " + accountId + ")"));
                Utils::Threading::runOnUIThread([this, accountId, devices]() {
                    knownDevicesChanged(toPlatformString(accountId), convertMap(devices));
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::ExportOnRingEnded>(
            [this](const std::string& accountId, int status, const std::string& pin) {
                MSG_SIG_("SIGNAL<ExportOnRingEnded> " + ("(accountId = " + accountId + ")"));
                Utils::Threading::runOnUIThread([this, accountId, status, pin]() {
                    exportOnRingEnded(toPlatformString(accountId), status, toPlatformString(pin));
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::RegisteredNameFound>(
            [this](const std::string &accountId, int status, const std::string &address, const std::string &name) {
                MSG_SIG_("SIGNAL<RegisteredNameFound> " + ("(accountId = " + accountId + " status = " + std::to_string(status) + ", address = " + address + ", name = " + name + ")"));
                Utils::Threading::runOnUIThread([this, accountId, status, address, name]() {
                    registeredNameFound(toPlatformString(accountId), translateLookupStatus(status), toPlatformString(address), toPlatformString(name));
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::VolatileDetailsChanged>(
            [this](const std::string& accountId, const std::map<std::string, std::string>& details) {
                Utils::Threading::runOnUIThread([this, accountId, details]() {
                    MSG_SIG_("SIGNAL<VolatileDetailsChanged> " + ("(accountId = " + accountId + ")"));
                    volatileDetailsChanged(toPlatformString(accountId), convertMap(details));
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::IncomingTrustRequest>(
            [this](const std::string& accountId, const std::string& from, const std::vector<uint8_t>& payload, time_t received) {
                Utils::Threading::runOnUIThread([this, accountId, from, payload, received]() {
                    MSG_SIG_("SIGNAL<IncomingTrustRequest> " + ("(accountId = " + accountId + ", from = " + from + " received = " + std::to_string(received) + ")"));
                    incomingTrustRequest(toPlatformString(accountId), toPlatformString(from), toPlatformString(std::string(payload.begin(), payload.end())), received);
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::ContactAdded>(
            [this](const std::string& accountId, const std::string& uri, bool confirmed) {
                MSG_SIG_("SIGNAL<ContactAdded> " + ("(accountId = " + accountId + ", uri = " + uri + " confirmed = " + std::to_string(confirmed) + ")"));
                Utils::Threading::runOnUIThread([this, accountId, uri, confirmed]() {
                    contactAdded(toPlatformString(accountId), toPlatformString(uri), confirmed);
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::ContactRemoved>(
            [this](const std::string& accountId, const std::string& uri, bool banned) {
                MSG_SIG_("SIGNAL<ContactRemoved> " + ("(accountId = " + accountId + ", uri = " + uri + " banned = " + std::to_string(banned) + ")"));
                Utils::Threading::runOnUIThread([this, accountId, uri, banned]() {
                    // not implemented
                });
            }),
        DRing::exportable_callback<DRing::ConfigurationSignal::DeviceRevocationEnded>(
            [this](const std::string& accountId, const std::string& device, int status) {
                MSG_SIG_("SIGNAL<DeviceRevocationEnded> " + ("(accountId = " + accountId + ", device = " + device + " status = " + std::to_string(status) + ")"));
                Utils::Threading::runOnUIThread([this, accountId, device, status]() {
                    deviceRevocationEnded(toPlatformString(accountId), toPlatformString(device), translateDeviceRevocationResult(status));
                });
            })
    };
    DRing::registerConfHandlers(configurationHandlers);
}

void
RingD::registerPresenceHandlers()
{
    presenceHandlers = {
        DRing::exportable_callback<DRing::PresenceSignal::NewBuddyNotification>(
            [this](const std::string& accountId, const std::string& uri, int status, const std::string& lineStatus) {
                MSG_SIG_("SIGNAL<IncomingAccountMessage> " + ("(accountId = " + accountId + ", uri = " + uri + " status = " + std::to_string(status) + ")"));
                Utils::Threading::runOnUIThread([this, accountId, uri, status, lineStatus]() {
                    newBuddyNotification(toPlatformString(accountId), toPlatformString(uri), status, toPlatformString(lineStatus));
                });
            })
    };
    DRing::registerPresHandlers(presenceHandlers);
}

void
RingD::registerVideoHandlers()
{
    using namespace Video;
    videoHandlers = {
        DRing::exportable_callback<DRing::VideoSignal::DeviceEvent>(
            [this]() {
                MSG_SIG_("SIGNAL<DeviceEvent>");
                Utils::Threading::runOnUIThread([this]() {
                    // not implemented
                });
            }),
        DRing::exportable_callback<DRing::VideoSignal::DecodingStarted>(
            [this](const std::string &callId, const std::string &shmPath, int width, int height, bool isMixer) {
                MSG_SIG_("SIGNAL<DecodingStarted> " + ("(callId = " + callId + ", width = " + std::to_string(width) + " height = " + std::to_string(height) + ")"));
                Utils::Threading::runOnUIThread([this, callId, shmPath, width, height, isMixer]() {
                    decodingStarted(toPlatformString(callId), toPlatformString(shmPath), width, height, isMixer);
                });
            }),
        DRing::exportable_callback<DRing::VideoSignal::DecodingStopped>(
            [this](const std::string &callId, const std::string &shmPath, bool isMixer) {
                MSG_SIG_("SIGNAL<DecodingStopped> " + ("(callId = " + callId + ")"));
                Utils::Threading::runOnUIThread([this, callId, shmPath, isMixer]() {
                    decodingStopped(toPlatformString(callId), toPlatformString(shmPath), isMixer);
                });
            }),
        DRing::exportable_callback<DRing::VideoSignal::DeviceAdded>(
            [this](const std::string& device) {
                MSG_SIG_("SIGNAL<DeviceAdded>");
                Utils::Threading::runOnUIThread([this, device]() {
                    // not implemented
                });
            }),
        DRing::exportable_callback<DRing::VideoSignal::ParametersChanged>(
            [this](const std::string& device) {
                MSG_SIG_("SIGNAL<ParametersChanged> " + ("(device = " + device + ")"));
                Utils::Threading::runOnUIThread([this, device]() {
                    parametersChanged(toPlatformString(device));
                });
            }),
        DRing::exportable_callback<DRing::VideoSignal::StartCapture>(
            [this](const std::string& device) {
                MSG_SIG_("SIGNAL<StartCapture> " + ("(device = " + device + ")"));
                Utils::Threading::runOnUIThread([this, device]() {
                    startCapture(toPlatformString(device));
                });
            }),
        DRing::exportable_callback<DRing::VideoSignal::StopCapture>(
            [this]() {
                MSG_SIG_("SIGNAL<StopCapture>");
                Utils::Threading::runOnUIThread([this]() {
                    stopCapture();
                });
            })
    };
    DRing::registerVideoHandlers(videoHandlers);
}

/* RingD slots (delegates) */
void
RingD::connectDelegates()
{
    /* attach delegates to events (slots to signals) for RingD */

    /* DRing */
    incomingCall += ref new IncomingCall(this, &RingD::onIncomingCall);
    stateChange += ref new StateChange(this, &RingD::onStateChange);
    incomingMessage += ref new IncomingMessage(this, &RingD::onIncomingMessage);
    incomingAccountMessage += ref new IncomingAccountMessage(this, &RingD::onIncomingAccountMessage);
    accountMessageStatusChanged += ref new AccountMessageStatusChanged(this, &RingD::onAccountMessageStatusChanged);
    registrationStateChanged += ref new RegistrationStateChanged(this, &RingD::onRegistrationStateChanged);
    accountsChanged += ref new AccountsChanged(this, &RingD::onAccountsChanged);
    nameRegistrationEnded += ref new NameRegistrationEnded(this, &RingD::onNameRegistrationEnded);
    knownDevicesChanged += ref new KnownDevicesChanged(this, &RingD::onKnownDevicesChanged);
    exportOnRingEnded += ref new ExportOnRingEnded(this, &RingD::onExportOnRingEnded);
    incomingTrustRequest += ref new IncomingTrustRequest(this, &RingD::onIncomingTrustRequest);
    contactAdded += ref new ContactAdded(this, &RingD::onContactAdded);
    deviceRevocationEnded += ref new DeviceRevocationEnded(this, &RingD::onDeviceRevocationEnded);
    decodingStarted += ref new DecodingStarted(this, &RingD::onDecodingStarted);
    decodingStopped += ref new DecodingStopped(this, &RingD::onDecodingStopped);
    parametersChanged += ref new ParametersChanged(this, &RingD::onParametersChanged);
    startCapture += ref new StartCapture(this, &RingD::onStartCapture);

    /* Client delegates */
    NetworkInformation::NetworkStatusChanged += ref new NetworkStatusChangedEventHandler(this, &RingD::InternetConnectionChanged);
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
RingD::onStateChange(String^ callId, CallStatus state, int code)
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
}

void
RingD::onIncomingMessage(String^ callId, String^ from, Map<String^, String^>^ payloads)
{
    handleIncomingMessage(from, payloads);
}

void
RingD::onIncomingAccountMessage(String^ accountId, String^ from, Map<String^, String^>^ payloads)
{
    handleIncomingMessage(from, payloads);
}

void
RingD::onAccountMessageStatusChanged(String^ accountId, uint64_t messageId, String^ to, int state)
{
    auto contactListModel = AccountsViewModel::instance->getContactList(Utils::toString(accountId));
    if (auto contact = contactListModel->findContactByRingId(to)) {
        auto conversation = contact->_conversation;
        if (conversation) {
            for (const auto& msg : conversation->_messages) {
                if (msg->MessageIdInteger == messageId &&
                    state == Utils::toUnderlyingValue(DRing::Account::MessageStates::SENT)) {
                    messageStatusUpdated(msg->MessageId, state);
                    msg->IsReceived = true;
                    contact->saveConversationToFile();
                }
            }
        }
    }
}

void
RingD::onRegistrationStateChanged(String^ accountId, String^ state, int detailsCode, String^ detailsStr)
{
    using namespace RingClientUWP::Utils;

    /* TODO: don't block the ui thread to get account details */
    auto accountDetails = std::make_shared<AccountDetails>();
    Threading::runOnWorkerThread(
        [this, state, accountId, accountDetails]() {
            *accountDetails = std::move(DRing::getAccountDetails(toString(accountId)));
        })->Completed = ref new AsyncActionCompletedHandler(
            [this, state, accountId, accountDetails](IAsyncAction^ asyncInfo, AsyncStatus asyncStatus) {
                Threading::runOnUIThread(
                    [this, state, accountId, accountDetails]() {
                        auto _state = toString(state);
                        if (_state == DRing::Account::States::INITIALIZING || isDeletingAccount) {
                            return;
                        }
                        parseAccountDetails(accountId, *accountDetails);
                        if (_state == DRing::Account::States::REGISTERED) {
                            if (auto accountItem = AccountItemsViewModel::instance->findItem(accountId))
                                accountItem->_registrationState = RegistrationState::REGISTERED;
                            registrationStateRegistered(toString(accountId));
                        }
                        else if (_state == DRing::Account::States::UNREGISTERED) {
                            if (auto accountItem = AccountItemsViewModel::instance->findItem(accountId))
                                accountItem->_registrationState = RegistrationState::UNREGISTERED;
                            registrationStateUnregistered(toString(accountId));
                        }
                        else if (_state == DRing::Account::States::TRYING) {
                            if (auto accountItem = AccountItemsViewModel::instance->findItem(accountId))
                                accountItem->_registrationState = RegistrationState::TRYING;
                            registrationStateTrying(toString(accountId));
                        }
                        else {
                            registrationStateErrorGeneric(toString(accountId));
                        }
                        if (isUpdatingAccount) {
                            onAccountUpdated();
                        }
                        Threading::runOnWorkerThread([this, _state]() {
                            if (_state == DRing::Account::States::REGISTERED) {
                                subscribeBuddies();
                            }
                        });
                    });
            });
}

void
RingD::onAccountsChanged()
{
    if (isDeletingAccount) {
        OnaccountDeleted();
    }
    else if (!isAddingAccount && !isDeletingAccount){
        auto allAccountDetails = getAllAccountDetails();
        Utils::Threading::runOnUIThread([this, allAccountDetails = std::move(allAccountDetails)]() {
            std::for_each(std::cbegin(allAccountDetails), std::cend(allAccountDetails),
                [=](std::pair<std::string, AccountDetails> acc) {
                parseAccountDetails(Utils::toPlatformString(acc.first), acc.second);
            });
        });
    }
}

void
RingD::onNameRegistrationEnded(String^ accountId, int state, String^ name)
{
    bool res = state == 0;

    if (!res)
        return;

    // old
    auto account = AccountsViewModel::instance->findItem(accountId);
    account->_username = name;

    // new
    auto accountItem = AccountItemsViewModel::instance->findItem(accountId);
    accountItem->_registeredName = name;

    mainPage->showLoadingOverlay(false, false);

    nameRegistered(res, accountId);
}

void
RingD::onKnownDevicesChanged(String^ accountId, Map<String^, String^>^ devices)
{
    if (isAddingDevice) {
        isAddingDevice = false;
        hideLoadingOverlay("This device has been successfully added", SuccessColor);
    }
    getKnownDevices(accountId);
}

void
RingD::onExportOnRingEnded(String^ accountId, int status, String^ pin)
{
    pin = Utils::isEmpty(pin) ? "Error bad password" : "Your generated PIN : " + pin;
    exportOnRingEnded(accountId, status, pin);
}

void
RingD::onIncomingTrustRequest(String^ accountId, String^ from, String^ payload, time_t received)
{
    auto parsedPayload = profile::parseContactRequestPayload(Utils::toString(payload));
    // First check if this TR has a corresponding contact. If not, add a contact
    // to the account's contact list with a trust status flag indicating that
    // it should be treated as a TR, and only appear in the contact request list.
    if (auto contactListModel = AccountsViewModel::instance->getContactList(Utils::toString(accountId))) {
        auto contact = contactListModel->findContactByRingId(from);

        // If the contact exists, we should check to see if we have previously
        // sent a TR to the peer. If, so we can accept this TR immediately.
        // Otherwise, if it is not already been trusted, we can ignore it completely.
        if (contact) {
            if (contact->_trustStatus == TrustStatus::CONTACT_REQUEST_SENT) {
                // get the vcard first
                auto vcard = contact->getVCard();
                vcard->setData(parsedPayload.at("VCARD"));
                vcard->completeReception();

                DRing::acceptTrustRequest(Utils::toString(accountId), Utils::toString(from));
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
            contact = contactListModel->addNewContact("", from, TrustStatus::INCOMING_CONTACT_REQUEST, false);
        }

        // The visible ring id will potentially be replaced by a username after a lookup
        RingD::instance->lookUpAddress(Utils::toString(accountId), from);

        auto vcard = contact->getVCard();
        vcard->setData(parsedPayload.at("VCARD"));
        vcard->completeReception();

        // The name is the ring id for now
        contact->_name = from;
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
}

void
RingD::onContactAdded(String^ accountId, String^ uri, bool confirmed)
{
    // If confirmed is false, we have just sent the TR and nothing need be done.
    // If confirmed is true, the sent TR has been accepted and we can change the
    // TrustStatus flag for the contact under the account_id, that matches the uri
    if (confirmed) {
        if (auto contactListModel = AccountsViewModel::instance->getContactList(Utils::toString(accountId))) {
            auto contact = contactListModel->findContactByRingId(uri);
            if (contact == nullptr) {
                contact = contactListModel->addNewContact(uri, uri, TrustStatus::TRUSTED, false);
                RingD::instance->lookUpAddress(Utils::toString(accountId), uri);
            }
            else {
                contact->_trustStatus = TrustStatus::TRUSTED;
            }

            SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
            SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);
            ContactRequestItemsViewModel::instance->update(ViewModel::NotifyStrings::notifyContactRequestItem);

            AccountsViewModel::instance->raiseContactDataModified(uri, contact);

            contactListModel->saveContactsToFile();
        }
    }
}

void
RingD::onDeviceRevocationEnded(String^ accountId, String^ device, DeviceRevocationResult status)
{
    switch (status) {
    case DeviceRevocationResult::INVALID_CERTIFICATE:
        hideLoadingOverlay("Certificate error while revoking device ID: " + device, ErrorColor);
        break;
    case DeviceRevocationResult::INVALID_PASSWORD:
        hideLoadingOverlay("Incorrect account password. Can't revoke device ID: " + device, ErrorColor);
        break;
    case DeviceRevocationResult::SUCCESS:
        getKnownDevices(accountId);
        hideLoadingOverlay("Device with ID: " + device + " has been successfully revoked", SuccessColor);
        break;
    }
}

void
RingD::onDecodingStarted(String^ callId, String^ shmPath, int width, int height, bool isMixer)
{
    Video::VideoManager::instance->rendererManager()->startedDecoding(callId, width, height);
    incomingVideoMuted(callId, false);
}

void
RingD::onDecodingStopped(String^ callId, String^ shmPath, bool isMixer)
{
    Video::VideoManager::instance->rendererManager()->removeRenderer(callId);
    incomingVideoMuted(callId, true);
}

void
RingD::onParametersChanged(String^ device)
{
    auto settings = DRing::getDeviceParams(Utils::toString(device));
    Video::VideoManager::instance->captureManager()->activeDevice->SetDeviceProperties(
        Utils::toPlatformString(settings["format"]), stoi(settings["width"]), stoi(settings["height"]), stoi(settings["rate"]));
}

void
RingD::onStartCapture(String^ device)
{
    Video::VideoManager::instance->captureManager()->InitializeCameraAsync(false);
    Video::VideoManager::instance->captureManager()->videoFrameCopyInvoker->Start();
}

void
RingD::onStopCapture()
{
    Video::VideoManager::instance->captureManager()->StopPreviewAsync();
    if (Video::VideoManager::instance->captureManager()->captureTaskTokenSource) {
        Video::VideoManager::instance->captureManager()->captureTaskTokenSource->cancel();
    }
    Video::VideoManager::instance->captureManager()->videoFrameCopyInvoker->Stop();
}