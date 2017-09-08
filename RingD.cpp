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

RingD::RingD()
{
    /* instantiate toaster */
    toaster_ = ToastNotificationManager::CreateToastNotifier();

    /* instantiate ringtone */
    ringtone_ = ref new Ringtone("default.wav");

    /*
    * get app's local folder
    * (e.g., C:\Users\<username>\AppData\Local\Packages\<packagename>\LocalState)
    */
    localFolder_ = Utils::toString(ApplicationData::Current->LocalFolder->Path);

    /*
    * - forward incoming signals to events on the UI thread
    * - bind the events to RingD delegates
    */
    connectDelegates();
}

void
RingD::InternetConnectionChanged(Object^ sender)
{
    hasInternet_ = Utils::hasInternet();
    networkChanged();
    connectivityChanged();
}

RingD::AccountDetailsBlob
RingD::getAllAccountDetails()
{
    RingD::AccountDetailsBlob allAccountDetails;
    std::vector<std::string> accountList = DRing::getAccountList();
    std::for_each(std::cbegin(accountList), std::cend(accountList), [&](const std::string& acc) {
        allAccountDetails[acc] = DRing::getAccountDetails(acc);
    });
    return allAccountDetails;
}

void
RingD::subscribeBuddies()
{
    for (auto accountItem : AccountItemsViewModel::instance->_itemsList) {
        if (Utils::hasInternet()) {
            auto contactListModel = AccountsViewModel::instance->getContactList(Utils::toString(accountItem->_id));
            for (auto contact : contactListModel->_contactsList) {
                if (!contact->subscribed_) {
                    MSG_("account: " + accountItem->_id + " subscribing to buddy presence for ringID: " + contact->ringID_);
                    subscribeBuddy(Utils::toString(accountItem->_id), Utils::toString(contact->ringID_), true);
                    contact->subscribed_ = true;
                }
            }
        }
    }
}

void
RingD::addContactFromDaemon(String^ accountId, Map<String^, String^>^ details)
{
    auto account = AccountItemsViewModel::instance->findItem(accountId);

    if (!account) {
        MSG_("Error when adding contact for accountId: " + accountId + " (can't find accountItem)");
        return;
    }

    using namespace RingClientUWP::Strings::Contact;
    using namespace Utils;

    auto ringId = details->Lookup("id");
    auto confirmed = details->Lookup("confirmed");

    auto _details = ref new Map<String^, String^>();

    _details->Insert(toPlatformString(URI), ringId);
    _details->Insert(toPlatformString(TRUSTED), confirmed);
    _details->Insert(toPlatformString(TYPE), toPlatformString(DRing::Account::ProtocolNames::RING));

    // from lookup later
    _details->Insert(toPlatformString(REGISTEREDNAME), "");

    // get displayName from vcard
    String^ displayName = "";
    _details->Insert(toPlatformString(DISPLAYNAME), displayName);

    // get alias from db if possible
    String^ alias = "";
    _details->Insert(toPlatformString(ALIAS), alias);

    auto contactItem = account->_contactItemList->addItem(_details);

    if (!contactItem) {
        MSG_("Error when adding contact for accountId: " + accountId + " (contactItem not created)");
        return;
    }

    // lookup registeredName
    RingD::instance->lookUpAddress(Utils::toString(accountId), ringId);

    MSG_("************NEW added contact from daemon: (id=" + ringId + ")");
    // TODO: emit client signal : contactItemAdded

}

void
RingD::addContactRequestFromDaemon(String^ accountId, Map<String^, String^>^ details)
{
    auto account = AccountItemsViewModel::instance->findItem(accountId);

    if (!account) {
        MSG_("Error when adding contact request for accountId: " + accountId + " (can't find accountItem)");
        return;
    }

    using namespace RingClientUWP::Strings::Contact;
    using namespace Utils;

    auto ringId = details->Lookup("from");
    auto timeReceived = details->Lookup("received");
    auto payload = details->Lookup("payload");

    auto _details = ref new Map<String^, String^>();

    _details->Insert(toPlatformString(URI), ringId);
    //_details->Insert(toPlatformString(TIMERECEIVED), timeReceived);
    //_details->Insert(toPlatformString(PAYLOAD), payload);
    _details->Insert(toPlatformString(TYPE), toPlatformString(DRing::Account::ProtocolNames::RING));

    // from lookup later
    _details->Insert(toPlatformString(REGISTEREDNAME), "");

    // get displayName from vcard
    String^ displayName = "";
    _details->Insert(toPlatformString(DISPLAYNAME), displayName);

    /* not yet implemented
    auto contactRequestItem = account->_contactRequestItemList->addItem(_details);

    if (!contactRequestItem) {
        MSG_("Error when adding contact request for accountId: " + accountId + " (contactRequestItem not created)");
        return;
    }

    // lookup registeredName
    RingD::instance->lookUpAddress(Utils::toString(accountId), ringId);
    */

    MSG_("************NEW added contact request from daemon: (id=" + ringId + ")");
    // TODO: emit client signal : contactRequestItemAdded

}

void
RingD::parseAccountDetails(String^ accountId, const AccountDetails& accountDetails)
{
    if (accountDetails.size() == 0)
        return;

    auto type = accountDetails.find(DRing::Account::ConfProperties::TYPE)->second;

    auto account_new = AccountItemsViewModel::instance->findItem(accountId);
    using namespace DRing::Account;

    {
        if (account_new) {
            account_new->SetDetails(accountId, Utils::convertMap(accountDetails));
            // TODO: emit client signal : accountItemUpdated (purposefully outside of SetDetails)
        }
        else {
            auto newItem = AccountItemsViewModel::instance->addItem(accountId, Utils::convertMap(accountDetails));
            if (isAddingAccount) {
                onAccountAdded(accountId);
            }
            auto contacts = DRing::getContacts(Utils::toString(accountId));
            for (auto& contactDetails : contacts) {
                addContactFromDaemon(accountId, Utils::convertMap(contactDetails));
            }
            if (type == ProtocolNames::RING) {
                auto contactRequests = DRing::getTrustRequests(Utils::toString(accountId));
                for (auto& contactRequestDetails : contactRequests) {
                    addContactRequestFromDaemon(accountId, Utils::convertMap(contactRequestDetails));
                }
            }
            // TODO: load group conversations
            // TODO: emit client signal : accountItemAdded
        }
    }

    if (type == "RING") {
        auto  ringID = accountDetails.find(ConfProperties::USERNAME)->second;
        if (!ringID.empty())
            ringID = ringID.substr(5);
        bool active = (accountDetails.find(ConfProperties::ENABLED)->second == ring::TRUE_STR) ? true : false;
        bool upnpState = (accountDetails.find(ConfProperties::UPNP_ENABLED)->second == ring::TRUE_STR) ? true : false;
        bool autoAnswer = (accountDetails.find(ConfProperties::AUTOANSWER)->second == ring::TRUE_STR) ? true : false;
        bool dhtPublicInCalls = (accountDetails.find(ConfProperties::DHT::PUBLIC_IN_CALLS)->second == ring::TRUE_STR) ? true : false;
        bool turnEnabled = (accountDetails.find(ConfProperties::TURN::ENABLED)->second == ring::TRUE_STR) ? true : false;
        auto turnAddress = accountDetails.find(ConfProperties::TURN::SERVER)->second;
        auto alias = accountDetails.find(ConfProperties::ALIAS)->second;
        auto deviceId = accountDetails.find(ConfProperties::RING_DEVICE_ID)->second;
        auto deviceName = accountDetails.find(ConfProperties::RING_DEVICE_NAME)->second;

        auto account = AccountsViewModel::instance->findItem(accountId);

        if (account) {
            account->name_ = Utils::toPlatformString(alias);
            account->_active = active;
            account->_upnpState = upnpState;
            account->accountType_ = Utils::toPlatformString(type);
            account->ringID_ = Utils::toPlatformString(ringID);
            account->_autoAnswer = autoAnswer;
            account->_turnEnabled = turnEnabled;
            account->_turnAddress = Utils::toPlatformString(turnAddress);
            account->_deviceId = Utils::toPlatformString(deviceId);
            account->_deviceName = Utils::toPlatformString(deviceName);

            // load contact requests for the account
            auto contactRequests = DRing::getTrustRequests(Utils::toString(accountId));
            if (auto contactListModel = AccountsViewModel::instance->getContactList(Utils::toString(accountId))) {
                for (auto& cr : contactRequests) {
                    auto ringId = cr.at("from");
                    auto timeReceived = cr.at("received");
                    auto payload = cr.at("payload");
                    auto fromP = Utils::toPlatformString(ringId);
                    auto contact = contactListModel->findContactByRingId(fromP);
                    if (contact == nullptr) {
                        contact = contactListModel->addNewContact(fromP, fromP, TrustStatus::INCOMING_CONTACT_REQUEST, false);

                        contactListModel->saveContactsToFile();
                        AccountsViewModel::instance->raiseUnreadContactRequest();

                        SmartPanelItemsViewModel::instance->refreshFilteredItemsList();
                        SmartPanelItemsViewModel::instance->update(ViewModel::NotifyStrings::notifySmartPanelItem);
                    }
                    else if (ContactRequestItemsViewModel::instance->findItem(contact) == nullptr) {
                        // The name is the ring id for now
                        contact->_name = Utils::toPlatformString(ringId);
                        // The visible ring id will potentially be replaced by a username after a lookup
                        RingD::instance->lookUpAddress(Utils::toString(accountId), Utils::toPlatformString(ringId));

                        auto vcard = contact->getVCard();
                        auto parsedPayload = profile::parseContactRequestPayload(payload);
                        std::string vcpart;
                        try {
                            vcpart.assign(parsedPayload.at("VCARD"));
                        }
                        catch (const std::exception& e) {
                            MSG_(e.what());
                        }
                        if (!vcpart.empty()) {
                            vcard->setData(vcpart);
                            vcard->completeReception();
                            contact->_displayName = Utils::toPlatformString(vcard->getPart("FN"));
                        }

                        auto newContactRequest = ref new ContactRequestItem();
                        newContactRequest->_contact = contact;
                        ContactRequestItemsViewModel::instance->itemsList->InsertAt(0, newContactRequest);

                        ContactRequestItemsViewModel::instance->refreshFilteredItemsList();
                        ContactRequestItemsViewModel::instance->update(ViewModel::NotifyStrings::notifyContactRequestItem);
                    }
                }
            }
            accountUpdated(account);
        }
        else {
            if (!ringID.empty()) {
                AccountsViewModel::instance->addRingAccount(alias,
                    ringID,
                    Utils::toString(accountId),
                    deviceId,
                    deviceName,
                    active,
                    upnpState,
                    autoAnswer,
                    dhtPublicInCalls,
                    turnEnabled,
                    turnAddress);
            }
        }
    }
    else { /* SIP */
        auto alias = accountDetails.find(ConfProperties::ALIAS)->second;
        bool active = (accountDetails.find(ConfProperties::ENABLED)->second == ring::TRUE_STR)
            ? true
            : false;
        auto sipHostname = accountDetails.find(ConfProperties::HOSTNAME)->second;
        auto sipUsername = accountDetails.find(ConfProperties::USERNAME)->second;
        auto sipPassword = accountDetails.find(ConfProperties::PASSWORD)->second;

        auto account = AccountsViewModel::instance->findItem(accountId);

        if (account) {
            account->name_ = Utils::toPlatformString(alias);
            account->accountType_ = Utils::toPlatformString(type);
            account->_sipHostname = Utils::toPlatformString(sipHostname);
            account->_sipUsername = Utils::toPlatformString(sipUsername);
            account->_sipPassword = Utils::toPlatformString(sipPassword);
            accountUpdated(account);
        }
        else {
            AccountsViewModel::instance->addSipAccount(alias,
                Utils::toString(accountId),
                active,
                sipHostname,
                sipUsername,
                sipPassword);
        }
    }
}

void
RingD::connectivityChanged()
{
    DRing::connectivityChanged();
}

void
RingD::sendAccountTextMessage(String^ message)
{
    tasks_.add_task([this, message]() {
        if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
            auto accountId = AccountItemsViewModel::instance->getSelectedAccountId();
            auto contact = item->_contact;
            auto uri = contact->ringID_;

            auto _accountId = Utils::toString(accountId);
            auto _uri = Utils::toString(uri);

            /* payload(s) */
            IBuffer^ buffUTF8 = CryptographicBuffer::ConvertStringToBinary(message, BinaryStringEncoding::Utf8);
            auto _message = Utils::getData(buffUTF8);
            std::map<std::string, std::string> _payload;
            _payload["text/plain"] = _message;

            auto sentToken = DRing::sendAccountTextMessage(_accountId, _uri, _payload);

            Utils::Threading::runOnUIThread([this, sentToken, contact, message]() {
                if (sentToken) {
                    contact->_conversation->addMessage(MSG_FROM_ME, message, std::time(nullptr), false, sentToken.ToString());
                    contact->saveConversationToFile();
                }
            });
        }
    });
}

void
RingD::sendSIPTextMessage(String^ message)
{
    tasks_.add_task([this, message]() {
        if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
            auto accountId = AccountItemsViewModel::instance->getSelectedAccountId();
            auto callId = item->_callId;

            auto _accountId = Utils::toString(accountId);
            auto _callId = Utils::toString(callId);
            auto _message = Utils::toString(message);
            std::map<std::string, std::string> _payload;
            _payload["text/plain"] = _message;

            DRing::sendTextMessage(_callId, _payload, _accountId, true /*not used*/);

            Utils::Threading::runOnUIThread([this, item, message]() {
                // No id generated for sip messages within a conversation, so let's generate one
                // so we can track message order.
                auto contact = item->_contact;
                auto messageId = Utils::toPlatformString(Utils::genID(0LL, 9999999999999999999LL));
                contact->_conversation->addMessage(MSG_FROM_ME, message, std::time(nullptr), false, messageId);

                contact->saveConversationToFile();
            });
        }
    });
}

void
RingD::sendSIPTextMessageVCF(std::string callID, std::map<std::string, std::string> message)
{
    tasks_.add_task([this, callID, message]() {
        auto accountId = AccountItemsViewModel::instance->getSelectedAccountId();
        auto _accountId = Utils::toString(accountId);
        auto _callId = callID;
        DRing::sendTextMessage(_callId, message, _accountId, true /*not used*/);
    });
}

void
RingD::createRINGAccount(String^ alias, String^ archivePassword, bool upnp, String^ registeredName)
{
    tasks_.add_task([this, alias, archivePassword, registeredName]() {
        auto _alias = Utils::toString(alias);
        auto _password = Utils::toString(archivePassword);
        auto _registeredName = Utils::toString(registeredName);

        std::map<std::string, std::string> accountDetails;
        accountDetails.insert(std::make_pair(DRing::Account::ConfProperties::ALIAS, _alias));
        accountDetails.insert(std::make_pair(DRing::Account::ConfProperties::ARCHIVE_PASSWORD, _password));
        accountDetails.insert(std::make_pair(DRing::Account::ConfProperties::TYPE, "RING"));
        accountDetails.insert(std::make_pair(DRing::Account::ConfProperties::UPNP_ENABLED, ring::TRUE_STR));

        Utils::Threading::runOnUIThread([this, registeredName]() {
            showLoadingOverlay(ResourceMananger::instance->getStringResource("_m_creating_account_"), SuccessColor);
            isAddingAccount = true;
            if (registeredName != "") {
                shouldRegister = true;
                nameToRegister = registeredName;
            }
        });

        DRing::addAccount(accountDetails);
    });
}

void
RingD::createSIPAccount(String^ alias, String^ sipPassword, String^ sipHostname, String^ sipusername)
{
    tasks_.add_task([this, alias, sipPassword, sipHostname, sipusername]() {
        auto _alias = Utils::toString(alias);
        auto _sipPassword = Utils::toString(sipPassword);
        auto _sipHostname = Utils::toString(sipHostname);
        auto _sipUsername = Utils::toString(sipusername);

        std::map<std::string, std::string> accountDetails;
        accountDetails.insert(std::make_pair(DRing::Account::ConfProperties::ALIAS, _alias));
        accountDetails.insert(std::make_pair(DRing::Account::ConfProperties::TYPE, "SIP"));
        accountDetails.insert(std::make_pair(DRing::Account::ConfProperties::PASSWORD, _sipPassword));
        accountDetails.insert(std::make_pair(DRing::Account::ConfProperties::HOSTNAME, _sipHostname));
        accountDetails.insert(std::make_pair(DRing::Account::ConfProperties::USERNAME, _sipUsername));

        Utils::Threading::runOnUIThread([this]() {
            showLoadingOverlay(ResourceMananger::instance->getStringResource("_m_creating_account_"), SuccessColor);
            isAddingAccount = true;
        });

        DRing::addAccount(accountDetails);
    });
}

void
RingD::refuseIncommingCall(String^ callId)
{
    tasks_.add_task([this, callId]() {
        auto _callId = Utils::toString(callId);
        DRing::refuse(_callId);
    });
}

void
RingD::acceptIncommingCall(String^ callId)
{
    tasks_.add_task([this, callId]() {
        auto _callId = Utils::toString(callId);
        DRing::accept(_callId);
    });
}

void
RingD::placeCall(RingClientUWP::Contact^ contact)
{
    tasks_.add_task([this, contact]() {
        auto _accountId = Utils::toString(contact->_accountIdAssociated);
        auto _ringId = Utils::toString(contact->ringID_);
        auto _sipUsername = Utils::toString(contact->_name);

        auto selectedAccountItem = AccountItemsViewModel::instance->_selectedItem;
        std::string callId;
        if (selectedAccountItem->_accountType == "RING")
            callId = DRing::placeCall(_accountId, "ring:" + _ringId);
        else if (selectedAccountItem->_accountType == "SIP")
            callId = DRing::placeCall(_accountId, "sip:" + _sipUsername);

        // TODO: this UI response should be called at state change?
        Utils::Threading::runOnUIThread([this, callId, _accountId, _ringId, _sipUsername, selectedAccountItem]() {
            if (auto contactListModel = AccountsViewModel::instance->getContactList(std::string(_accountId))) {
                Contact^ contact;
                if (selectedAccountItem->_accountType == "RING")
                    contact = contactListModel->findContactByRingId(Utils::toPlatformString(_ringId));
                else if (selectedAccountItem->_accountType == "SIP")
                    contact = contactListModel->findContactByName(Utils::toPlatformString(_sipUsername));
                if (auto item = SmartPanelItemsViewModel::instance->findItem(contact)) {
                    item->_callId = Utils::toPlatformString(callId);
                    if (!callId.empty())
                        callPlaced(Utils::toPlatformString(callId));
                }
            }
        });
    });
}

void
RingD::pauseCall(const std::string & callId)
{
    tasks_.add_task([this, callId]() {
       DRing::hold(callId);
    });
}

void
RingD::pauseCall(String ^ callId)
{
    tasks_.add_task([this, callId]() {
        auto _callId = Utils::toString(callId);
        DRing::hold(_callId);
    });
}

void
RingD::unPauseCall(const std::string & callId)
{
    tasks_.add_task([this, callId]() {
        DRing::unhold(callId);
    });
}

void
RingD::unPauseCall(String ^ callId)
{
    tasks_.add_task([this, callId]() {
        auto _callId = Utils::toString(callId);
        DRing::unhold(_callId);
    });
}

void
RingD::hangUpCall2(String ^ callId)
{
    tasks_.add_task([this, callId]() {
        auto _callId = Utils::toString(callId);
        DRing::hangUp(_callId);
    });
}

void
RingD::cancelOutGoingCall(String ^ callId)
{
    hangUpCall2(callId);
}

void
RingD::getKnownDevices(String^ accountId)
{
    tasks_.add_task([this, accountId]() {
        auto _accountId = Utils::toString(accountId);

        auto devicesList = DRing::getKnownRingDevices(_accountId);
        if (devicesList.empty())
            return;

        Utils::Threading::runOnUIThread([=]() {
            devicesListRefreshed(Utils::convertMap(devicesList));
        });
    });
}

void
RingD::ExportOnRing(String ^ accountId, String ^ password)
{
    tasks_.add_task([this, accountId, password]() {
        auto _accountId = Utils::toString(accountId);
        auto _password = Utils::toString(password);
        DRing::exportOnRing(_accountId, _password);
    });
}

void
RingD::updateAccount(String^ accountId)
{
    tasks_.add_task([this, accountId]() {
        auto _accountId = Utils::toString(accountId);

        auto accountItem = AccountItemsViewModel::instance->findItem(accountId);
        std::map<std::string, std::string> accountDetails = DRing::getAccountDetails(_accountId);
        std::map<std::string, std::string> accountDetailsOld(accountDetails);
        std::vector<std::map<std::string, std::string>> credentials = DRing::getCredentials(_accountId);
        std::vector<std::map<std::string, std::string>> credentialsOld(credentials);

        accountDetails[DRing::Account::ConfProperties::ALIAS] = Utils::toString(accountItem->_alias);
        accountDetails[DRing::Account::ConfProperties::ENABLED] = (accountItem->_enabled) ? ring::TRUE_STR : ring::FALSE_STR;
        accountDetails[DRing::Account::ConfProperties::AUTOANSWER] = (accountItem->_autoAnswerEnabled) ? ring::TRUE_STR : ring::FALSE_STR;

        bool userNameAdded = false;
        auto newUsername = accountItem->_registeredName;
        if (accountDetails[DRing::Account::ConfProperties::TYPE] == "RING") {
            if (accountItem->_registeredName != "") {
                auto oldUsername = getRegisteredName(accountItem->_id);
                userNameAdded = !(newUsername == oldUsername);
            }
            accountDetails[DRing::Account::ConfProperties::RING_DEVICE_NAME] = Utils::toString(accountItem->_deviceName);
            accountDetails[DRing::Account::ConfProperties::UPNP_ENABLED] = (accountItem->_upnpEnabled) ? ring::TRUE_STR : ring::FALSE_STR;
            accountDetails[DRing::Account::ConfProperties::DHT::PUBLIC_IN_CALLS] = (accountItem->_dhtPublicInCalls) ? ring::TRUE_STR : ring::FALSE_STR;
            accountDetails[DRing::Account::ConfProperties::TURN::ENABLED] = (accountItem->_turnEnabled) ? ring::TRUE_STR : ring::FALSE_STR;
            accountDetails[DRing::Account::ConfProperties::TURN::SERVER] = Utils::toString(accountItem->_turnAddress);
        }
        else {
            accountDetails[DRing::Account::ConfProperties::HOSTNAME] = Utils::toString(accountItem->_sipHostname);
            credentials.at(0)[DRing::Account::ConfProperties::PASSWORD] = Utils::toString(accountItem->_sipPassword);
            credentials.at(0)[DRing::Account::ConfProperties::USERNAME] = Utils::toString(accountItem->_sipUsername);
        }

        bool detailsChanged = (accountDetails != accountDetailsOld || userNameAdded);
        bool credentialsChanged = (credentials != credentialsOld);

        if (!detailsChanged && !credentialsChanged) {
            onAccountUpdated();
            return;
        }

        Utils::Threading::runOnUIThread([this]() {
            isUpdatingAccount = true;
            setOverlayStatusText(ResourceMananger::instance->getStringResource("_m_updating_account_"), SuccessColor);
            mainPage->showLoadingOverlay(true, true);
        });

        if (credentialsChanged) {
            DRing::setCredentials(_accountId, credentials);
        }

        DRing::setAccountDetails(_accountId, accountDetails);
        if (userNameAdded) {
            registerName(accountItem->_id, "", accountItem->_registeredName);
        }

        Configuration::UserPreferences::instance->save();
        Configuration::UserPreferences::instance->saveProfileToVCard();
    });
}

void
RingD::deleteAccount(String ^ accountId)
{
    Utils::Threading::runOnUIThread([this, accountId]() {
        isDeletingAccount = true;
        setOverlayStatusText(ResourceMananger::instance->getStringResource("_m_deleting_account_"), "#ffff0000");
        mainPage->showLoadingOverlay(true, true);
        AccountItemsViewModel::instance->removeItem(accountId);
    });

    tasks_.add_task([this, accountId]() {
        auto _accountId = Utils::toString(accountId);
        DRing::removeAccount(_accountId);
    });
}

void
RingD::registerThisDevice(String ^ pin, String ^ archivePassword)
{
    Utils::Threading::runOnUIThread([this]() {
        showLoadingOverlay(ResourceMananger::instance->getStringResource("_m_creating_account_"), SuccessColor);
        isAddingAccount = true;
    });

    tasks_.add_task([this, pin, archivePassword]() {
        auto _pin = Utils::toString(pin);
        auto _password = Utils::toString(archivePassword);

        std::map<std::string, std::string> deviceDetails;
        deviceDetails.insert(std::make_pair(DRing::Account::ConfProperties::TYPE, "RING"));
        deviceDetails.insert(std::make_pair(DRing::Account::ConfProperties::ARCHIVE_PIN, _pin));
        deviceDetails.insert(std::make_pair(DRing::Account::ConfProperties::ARCHIVE_PASSWORD, _password));

        DRing::addAccount(deviceDetails);
    });
}

void
RingD::muteVideo(String ^ callId, bool muted)
{
    tasks_.add_task([this, callId, muted]() {
        auto _callId = Utils::toString(callId);
        DRing::muteLocalMedia(_callId, DRing::Media::Details::MEDIA_TYPE_VIDEO, muted);
    });
}

void
RingD::muteAudio(const std::string& callId, bool muted)
{
    tasks_.add_task([this, callId, muted]() {
        auto _callId = callId;
        DRing::muteLocalMedia(_callId, DRing::Media::Details::MEDIA_TYPE_AUDIO, muted);
    });
}

void
RingD::lookUpName(const std::string& accountId, String^ name)
{
    tasks_.add_task([this, accountId, name]() {
        auto _accountId = accountId;
        auto _name = Utils::toString(name);
        DRing::lookupName(_accountId, "", _name);
    });
}

void
RingD::lookUpAddress(const std::string& accountId, String^ address)
{
    tasks_.add_task([this, accountId, address]() {
        auto _accountId = accountId;
        auto _address = Utils::toString(address);
        DRing::lookupAddress(_accountId, "", _address);
    });
}

void
RingD::revokeDevice(const std::string& accountId, const std::string& password, const std::string& deviceId)
{
    Utils::Threading::runOnUIThread([this, deviceId]() {
        auto msg = ResourceMananger::instance->getStringResource("_m_revoking_device_");
        showLoadingOverlay(msg + Utils::toPlatformString(deviceId), SuccessColor);
    });

    tasks_.add_task([this, accountId, password, deviceId]() {
        auto _accountId = accountId;
        auto _password = password;
        auto _deviceId = deviceId;

        DRing::revokeDevice(_accountId, _password, _deviceId);
    });
}

void
RingD::registerName(String^ accountId, String^ password, String^ username)
{
    auto accountItem = AccountItemsViewModel::instance->findItem(accountId);
    if (!accountItem || accountItem->_accountType != "RING" || !accountItem->_ringId)
        return;

    tasks_.add_task([this, accountId, password, username]() {
        auto _accountId = Utils::toString(accountId);
        auto _password = Utils::toString(password);
        auto _username = Utils::toString(username);

        bool result;

        result = DRing::registerName(_accountId, _password, _username);
    });
}

void
RingD::subscribeBuddy(const std::string& accountId, const std::string& uri, bool flag)
{
    tasks_.add_task([this, accountId, uri, flag]() {
        auto _accountId = accountId;
        auto _uri = uri;

        DRing::subscribeBuddy(_accountId, _uri, true);
    });
}

void
RingD::removeContact(const std::string& accountId, const std::string& uri)
{
    tasks_.add_task([this, accountId, uri]() {
        auto _accountId = accountId;
        auto _uri = uri;
        DRing::removeContact(_accountId, _uri, false);
    });
}

void
RingD::sendContactRequest(const std::string& accountId, const std::string& uri, const std::string& payload)
{
    tasks_.add_task([this, accountId, uri, payload]() {
        auto _accountId = accountId;
        auto _uri = uri;
        auto _payload = payload;
        std::vector<uint8_t> payload(_payload.begin(), _payload.end());
        DRing::sendTrustRequest(_accountId, _uri, payload);
    });
}

void
RingD::ShowCallToast(bool background, String^ callId, String^ name)
{
    auto item = SmartPanelItemsViewModel::instance->findItem(callId);

    if (!item)
        return;

    auto bestName = name != nullptr ? name : item->_contact->_bestName2;
    String^ avatarUri = item->_contact->_avatarImage;
    if (avatarUri == nullptr || avatarUri == L" ")
        avatarUri = ref new String(L"ms-appx:///Assets/TESTS/contactAvatar.png");

    String^ xml =
        "<toast scenario='incomingCall'>"
        "<visual> "
        "<binding template='ToastGeneric'>"
        "<image placement='appLogoOverride' hint-crop='circle' src='";

    xml += avatarUri + "'/>";

    xml += "<text>" + bestName + "</text>";

    auto acceptArgString = "a:" + callId;
    auto refuseArgString = "r:" + callId;

    xml +=
        "</binding>"
        "</visual>"
        "<actions>"
        "<action activationType='foreground' arguments= '" + acceptArgString + "' content='Accept' />"
        "<action activationType='foreground' arguments= '" + refuseArgString + "' content='Refuse' />"
        "</actions>"
        "<audio silent='true' loop='true'/></toast>";

    auto doc = ref new XmlDocument();
    doc->LoadXml(xml);

    toastCall_ = ref new ToastNotification(doc);
    toastCall_->Dismissed += ref new TypedEventHandler<ToastNotification ^, ToastDismissedEventArgs ^>(
        [this](ToastNotification ^sender, ToastDismissedEventArgs ^args) {callToastPopped_ = false; });
    toastCall_->Failed += ref new TypedEventHandler<ToastNotification ^, ToastFailedEventArgs ^>(
        [this](ToastNotification ^sender, ToastFailedEventArgs ^args) {callToastPopped_ = false; });
    toaster_->Show(toastCall_);
    callToastPopped_ = true;
}

void
RingD::ShowIMToast(bool background, String^ from, String^ payload, String^ name)
{
    auto item = SmartPanelItemsViewModel::instance->findItemByRingID(from);

    if (!item)
        return;

    IBuffer^ buffUTF8 = CryptographicBuffer::ConvertStringToBinary(payload, BinaryStringEncoding::Utf8);
    auto binMessage = Utils::toPlatformString(Utils::getData(buffUTF8));

    auto bestName = name != nullptr ? name : item->_contact->_bestName2;
    String^ avatarUri = item->_contact->_avatarImage;
    if (avatarUri == nullptr || avatarUri == L" ")
        avatarUri = ref new String(L"ms-appx:///Assets/TESTS/contactAvatar.png");

    String^ xml =
        "<toast scenario='incomingMessage' duration='short'>"
        "<visual> "
        "<binding template='ToastGeneric'>"
        "<image placement='appLogoOverride' hint-crop='circle' src='";

    xml += avatarUri + "'/>";

    xml += "<text>" + bestName + "</text>";

    xml += "<text>" + binMessage + "</text>";

    xml +=
        "</binding>"
        "</visual>";
        /*"<actions>"
        "<input id='replyTextBox' type='text' placeholderContent='Type a reply'/>"
        "<action content='Reply' arguments='action=reply&amp;convId=9318' activationType='background' hint-inputId='replyTextBox'/>"
        "</actions>";*/

    xml += "<audio src='ms-appx:///Assets/ringtone_notify.wav' loop='false'/></toast>";

    auto doc = ref new XmlDocument();
    doc->LoadXml(xml);

    toastText_ = ref new ToastNotification(doc);
    toaster_->Show(toastText_);
}

void
RingD::HideToast(ToastNotification^ toast)
{
    toaster_->Hide(toast);
}

void
RingD::handleIncomingMessage(String^ from, Map<String^, String^>^ payloads)
{
    from = Utils::TrimRingId2(from);

    auto item = SmartPanelItemsViewModel::instance->findItemByRingID(from);
    if (!item) {
        WNG_("VCard - item not found!");
        return;
    }

    Contact^ contact = item->_contact;

    /* TODO: Use Platform Maps for VCard treatment here */
    auto _payloads = Utils::convertMap(payloads);
    static const unsigned int profileSize = profile::PROFILE_VCF.size();
    for (auto i : _payloads) {
        if (i.first.compare(0, profileSize, profile::PROFILE_VCF) == 0) {
            contact->getVCard()->receiveChunk(i.first, i.second);
            return;
        }
    }
}

void
RingD::onAccountAdded(String^ accountId)
{
    if (shouldRegister) {
        shouldRegister = false;
        registerName(accountId, "", nameToRegister);
    }

    if (isAddingAccount) {
        isAddingAccount = false;
        hideLoadingOverlay("Account created successfully", SuccessColor, 2000);
        selectAccount(accountId);
        Configuration::UserPreferences::instance->save();
        Configuration::UserPreferences::instance->saveProfileToVCard();
    }
}

void
RingD::onAccountUpdated()
{
    isUpdatingAccount = false;
    hideLoadingOverlay("Account updated successfully", SuccessColor, 500);
}

void
RingD::OnaccountDeleted()
{
    isDeletingAccount = false;

    auto accountList = DRing::getAccountList();
    if (accountList.size() == 0) {
        auto configFile = RingD::instance->getLocalFolder() + ".config\\dring.yml";
        Utils::fileDelete(configFile);
        Utils::Threading::runOnUIThreadDelayed(100, [this]() {
            summonWizard();
        });
    }
    else {
        hideLoadingOverlay("Account deleted successfully", SuccessColor, 500);
    }
}

void
RingD::selectAccount(int index)
{
    selectAccountItemIndex(index);
}

void
RingD::selectAccount(String ^ accountId)
{
    selectAccountItemIndex(AccountItemsViewModel::instance->getIndex(accountId));
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

    setOverlayStatusText("Starting Ring...", "#ff000000");

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

    IAsyncAction^ action = ThreadPool::RunAsync(ref new WorkItemHandler([=](IAsyncAction^ spAction)
    {
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::Normal,
            ref new DispatchedHandler([=]() {
            if (!isInWizard) {
                setOverlayStatusText("Loading from config...", "#ff000000");
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
            /* at this point the config.yml is safe. */
            std::string tokenFile = localFolder_ + "\\creation.token";
            if (fileExists(tokenFile)) {
                fileDelete(tokenFile);
            }

            if (!isInWizard) {
                hideLoadingOverlay("Ring started successfully", SuccessColor, 1000);
            }

            // Database can be loaded here

            // Initial loading of account from the daemon
            auto allAccountDetails = getAllAccountDetails();
            Utils::Threading::runOnUIThread([this, allAccountDetails = std::move(allAccountDetails)]() {
                std::for_each(std::cbegin(allAccountDetails), std::cend(allAccountDetails),
                    [=](std::pair<std::string, AccountDetails> acc) {
                        parseAccountDetails(Utils::toPlatformString(acc.first), acc.second);
                    });
                Configuration::UserPreferences::instance->load();
            });

            while (daemonRunning) {
                DRing::pollEvents();
                tasks_.dequeue_tasks();
                Sleep(5);
            }
        }
    },Platform::CallbackContext::Any), WorkItemPriority::High);
}

std::string
RingD::getLocalFolder()
{
    return localFolder_ + Utils::toString("\\");
}

void
RingD::showLoadingOverlay(String^ text, String^ color)
{
    Utils::Threading::runOnUIThread([=]() {
        setOverlayStatusText(text, color);
        mainPage->showLoadingOverlay(true, true);
    });
}

void
RingD::hideLoadingOverlay(String^ text, String^ color, int delayInMilliseconds)
{
    Utils::Threading::runOnUIThread([=]() { setOverlayStatusText(text, color); });
    Utils::Threading::runOnUIThreadDelayed(delayInMilliseconds, [=]() {
        mainPage->showLoadingOverlay(false, false);
    });
}

std::map<std::string, std::string>
RingD::getVolatileAccountDetails(RingClientUWP::Account^ account)
{
    return DRing::getVolatileAccountDetails(Utils::toString(account->accountID_));
}

String^
RingD::getRegisteredName(String^ accountId)
{
    auto volatileAccountDetails = DRing::getVolatileAccountDetails(Utils::toString(accountId));
    return Utils::toPlatformString(volatileAccountDetails[DRing::Account::VolatileProperties::REGISTERED_NAME]);
}

CallStatus
RingD::translateCallStatus(String^ state)
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

LookupStatus
RingD::translateLookupStatus(int status)
{
    return Utils::toEnum<LookupStatus>(status);
}

DeviceRevocationResult
RingD::translateDeviceRevocationResult(int status)
{
    return Utils::toEnum<DeviceRevocationResult>(status);
}

String^
RingD::getUserName()
{
    auto users = User::FindAllAsync();
    return nullptr;
}

void
RingD::startSmartInfo(int refresh)
{
    tasks_.add_task([this, refresh]() {
        DRing::startSmartInfo(refresh);
    });
}

void
RingD::stopSmartInfo()
{
    tasks_.add_task([this]() {
        DRing::stopSmartInfo();
    });
}

void
RingD::setFullScreenMode()
{
    if (ApplicationView::GetForCurrentView()->TryEnterFullScreenMode()) {
        MSG_("TryEnterFullScreenMode succeeded");
        fullScreenToggled(true);
    }
    else {
        ERR_("TryEnterFullScreenMode failed");
    }
}

void
RingD::setWindowedMode()
{
    ApplicationView::GetForCurrentView()->ExitFullScreenMode();
    MSG_("ExitFullScreenMode");
    fullScreenToggled(false);
}

void
RingD::toggleFullScreen()
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

void
RingD::raiseWindowResized(float width, float height)
{
    windowResized(width, height);
}

void
RingD::raiseVCardUpdated(RingClientUWP::Contact^ contact)
{
    vCardUpdated(contact);
}

void
RingD::raiseShareRequested()
{
    shareRequested();
}
