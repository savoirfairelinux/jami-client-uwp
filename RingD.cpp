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
RingD::InternetConnectionChanged(Platform::Object^ sender)
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

    if (!account)
        return;

    using namespace RingClientUWP::Strings::Contact;
    using namespace Utils;

    auto ringId = details->Lookup("id");

    Map<String^, String^>^ _details;

    _details->Insert(toPlatformString(URI), ringId);
    _details->Insert(toPlatformString(TRUSTED), details->Lookup("confirmed"));
    _details->Insert(toPlatformString(TYPE), toPlatformString(DRing::Account::ProtocolNames::RING));

    _details->Insert(toPlatformString(REGISTEREDNAME), ""); // from lookup later

    // get displayName vcard
    String^ displayName = "";
    _details->Insert(toPlatformString(DISPLAYNAME), displayName); // from vcard

    // get alias from db if possible
    String^ alias = "";
    _details->Insert(toPlatformString(ALIAS), alias); // from db

    auto contactItem = account->_contactItemList->addItem(_details);

    // lookup registeredName
    RingD::instance->lookUpAddress(Utils::toString(accountId), ringId);

}

void
RingD::parseAccountDetails(const AccountDetailsBlob& allAccountDetails)
{
    std::for_each(std::cbegin(allAccountDetails), std::cend(allAccountDetails),
        [=](std::pair<std::string, AccountDetails> acc) {
        auto accountId = acc.first;
        auto accountDetails = acc.second;
        auto type = accountDetails.find(DRing::Account::ConfProperties::TYPE)->second;

        auto account_new = AccountItemsViewModel::instance->findItem(Utils::toPlatformString(accountId));
        using namespace DRing::Account;

        {
            if (account_new) {
                MSG_("************NEW setting account details");
                account_new->SetDetails(Utils::toPlatformString(accountId), Utils::convertMap(accountDetails));
                // emit signal : accountUpdated
            }
            else {
                MSG_("************NEW adding account");
                AccountItemsViewModel::instance->addItem(Utils::toPlatformString(accountId), Utils::convertMap(accountDetails));
                if (isAddingAccount) {
                    onAccountAdded(accountId);
                }
                // TODO: load contacts for the account
                auto contacts = DRing::getContacts(accountId);
                for (auto& contact : contacts) {
                    auto id = contact.at("id");
                    // add contact here
                    MSG_("************NEW contact: (id=" + id + ")");
                }
                if (type == ProtocolNames::RING) {
                    // TODO: load contact requests for the account
                    auto contactRequests = DRing::getTrustRequests(accountId);
                    for (auto& contactRequest : contactRequests) {
                        auto ringId = contactRequest.at("from");
                        auto timeReceived = contactRequest.at("received");
                        auto payload = contactRequest.at("payload");
                        MSG_("************NEW contactRequest: (from=" + ringId + ", t=" + timeReceived + ", p=" + payload + ")");
                    }
                }
                // emit signal : accountLoaded
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

            auto account = AccountsViewModel::instance->findItem(Utils::toPlatformString(accountId));

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
                auto contactRequests = DRing::getTrustRequests(accountId);
                if (auto contactListModel = AccountsViewModel::instance->getContactList(std::string(accountId))) {
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
                            RingD::instance->lookUpAddress(accountId, Utils::toPlatformString(ringId));

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
                        accountId,
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
                AccountsViewModel::instance->addSipAccount(alias,
                    accountId,
                    active,
                    sipHostname,
                    sipUsername,
                    sipPassword);
            }
        }
    });
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
    Utils::Threading::runOnUIThread([this]() {
        isDeletingAccount = true;
        setOverlayStatusText(ResourceMananger::instance->getStringResource("_m_deleting_account_"), "#ffff0000");
        mainPage->showLoadingOverlay(true, true);
    });

    tasks_.add_task([this, accountId]() {
        auto _accountId = Utils::toString(accountId);

        DRing::removeAccount(_accountId);

        Utils::Threading::runOnUIThread([this, accountId]() {
            AccountItemsViewModel::instance->removeItem(accountId);
        });
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
    auto account = AccountsViewModel::instance->findItem(accountId);
    if (!account || account->accountType_ != "RING" || !account->ringID_)
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

    static const unsigned int profileSize = profile::PROFILE_VCF.size();
    for (auto i : payloads) {
        if (i.first.compare(0, profileSize, profile::PROFILE_VCF) == 0) {
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
        DRing::exportable_callback<DRing::CallSignal::IncomingCall>([this](
                    const std::string& accountId,
                    const std::string& callId,
                    const std::string& from)
        {
            MSG_("<IncomingCall>");
            MSG_("accountId = " + accountId);
            MSG_("callId = " + callId);
            MSG_("from = " + from);

            auto _accountId = toPlatformString(accountId);
            auto _callId = toPlatformString(callId);
            auto _from = toPlatformString(from);

            /* fix some issue in the daemon --> <...@...> */
            _from = Utils::TrimRingId(_from);

            CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
            {
                auto accountItem = AccountItemsViewModel::instance->findItem(_accountId);
                if (auto contactListModel = AccountsViewModel::instance->getContactList(std::string(accountId))) {
                    Contact^ contact;
                    if (accountItem->_accountType == "RING")
                        contact = contactListModel->findContactByRingId(_from);
                    else if (accountItem->_accountType == "SIP")
                        contact = contactListModel->findContactByName(_from);
                    if (contact) {
                        auto item = SmartPanelItemsViewModel::instance->findItem(contact);
                        if (item)
                            item->_callId = _callId;
                    }
                }

                if (accountItem->_autoAnswerEnabled) {
                    incomingCall(_accountId, _callId, _from);
                    acceptIncommingCall(_callId);
                    stateChange(_callId, CallStatus::AUTO_ANSWERING, 0);
                }
                else {
                    incomingCall(_accountId, _callId, _from);
                    stateChange(_callId, CallStatus::INCOMING_RINGING, 0);
                }
            }));
        }),
        DRing::exportable_callback<DRing::CallSignal::SmartInfo>([this](
            const std::map<std::string, std::string>& info)
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
        DRing::exportable_callback<DRing::DebugSignal::MessageSend>([&](const std::string& msg)
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
RingD::onAccountAdded(const std::string& accountId)
{
    if (shouldRegister) {
        shouldRegister = false;
        registerName(Utils::toPlatformString(accountId), "", nameToRegister);
    }

    hideLoadingOverlay("Account created successfully", SuccessColor, 2000);

    if (isAddingAccount) {
        isAddingAccount = false;
        selectAccount(Utils::toPlatformString(accountId));
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
    if (AccountItemsViewModel::instance->_itemsList->Size == 0) {
        auto configFile = RingD::instance->getLocalFolder() + ".config\\dring.yml";
        Utils::fileDelete(configFile);
        Utils::Threading::runOnUIThreadDelayed(100,[this]() {summonWizard(); });
    }
    else {
        hideLoadingOverlay("Account deleted successfully", SuccessColor, 500);
    }
}

void
RingD::selectAccount(int index)
{
    selectIndex(index);
}

void
RingD::selectAccount(String ^ accountId)
{
    selectIndex(AccountItemsViewModel::instance->getIndex(accountId));
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

            auto allAccountDetails = getAllAccountDetails();
            Utils::Threading::runOnUIThread([this, allAccountDetails = std::move(allAccountDetails)]() {
                parseAccountDetails(allAccountDetails);
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

RingD::RingD()
{
    startingStatus_ = StartingStatus::NORMAL;
    toaster_ = ToastNotificationManager::CreateToastNotifier();
    ringtone_ = ref new Ringtone("default.wav");
    NetworkInformation::NetworkStatusChanged += ref new NetworkStatusChangedEventHandler(this, &RingD::InternetConnectionChanged);
    this->stateChange += ref new StateChange(this, &RingD::onStateChange);
    localFolder_ = Utils::toString(ApplicationData::Current->LocalFolder->Path);
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

String^
RingD::getUserName()
{
    auto users = User::FindAllAsync();
    return nullptr;
}

void
RingD::startSmartInfo(int refresh)
{
    DRing::startSmartInfo(refresh);
}

void
RingD::stopSmartInfo()
{
    DRing::stopSmartInfo();
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
