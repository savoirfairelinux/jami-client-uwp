/***************************************************************************
 * Copyright (C) 2016 by Savoir-faire Linux                                *
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
#include "AccountsViewModel.h"

using namespace RingClientUWP;
using namespace ViewModel;

using namespace Windows::UI::Core;

AccountsViewModel::AccountsViewModel()
{
    accountsList_ = ref new Vector<Account^>();
    contactListModels_ = ref new Map<String^, ContactListModel^>();

    RingD::instance->incomingAccountMessage +=
        ref new RingClientUWP::IncomingAccountMessage(this, &AccountsViewModel::OnincomingAccountMessage);
    RingD::instance->incomingMessage +=
        ref new RingClientUWP::IncomingMessage(this, &AccountsViewModel::OnincomingMessage);
}

void
AccountsViewModel::raiseContactAdded(String^ accountId, Contact ^ contact)
{
    contactAdded(accountId, contact);
}

void
AccountsViewModel::raiseContactDeleted(String^ accountId, Contact ^ contact)
{
    contactDeleted(accountId, contact);
}

void
AccountsViewModel::raiseContactDataModified(String^ accountId, Contact ^ contact)
{
    contactDataModified(accountId, contact);
}

void
AccountsViewModel::raiseUnreadContactRequest()
{
    newUnreadContactRequest();
}

Account^
AccountsViewModel::addRingAccount(  std::string& alias,
                                    std::string& ringID,
                                    std::string& accountID,
                                    std::string& deviceId,
                                    std::string& deviceName,
                                    bool active,
                                    bool upnpState,
                                    bool autoAnswer,
                                    bool dhtPublicInCalls,
                                    bool turnEnabled,
                                    std::string& turnAddress)
{
    auto account = ref new Account( Utils::toPlatformString(alias),
                                    Utils::toPlatformString(ringID),
                                    "RING",
                                    Utils::toPlatformString(accountID),
                                    Utils::toPlatformString(deviceId),
                                    Utils::toPlatformString(deviceName),
                                    active,
                                    upnpState,
                                    autoAnswer,
                                    dhtPublicInCalls,
                                    turnEnabled,
                                    Utils::toPlatformString(turnAddress),
                                    "" /* sip hostame not used with ring account */,
                                    "" /* sip username not used with ring account */,
                                    "" /* sip password not used with ring */ );
    RingD::instance->lookUpAddress(accountID, Utils::toPlatformString(ringID));
    accountsList_->InsertAt(0, account);
    contactListModels_->Insert(account->accountID_, ref new ContactListModel(account->accountID_));
    accountAdded(account);
    return account;
}

Account^
AccountsViewModel::addSipAccount(   std::string& alias,
                                    std::string& accountID,
                                    bool active,
                                    std::string& sipHostname,
                                    std::string& sipUsername,
                                    std::string& sipPassword)
{
    auto account = ref new Account( Utils::toPlatformString(alias),
                                    "" /* ring Id not used with sip */ ,
                                    "SIP",
                                    Utils::toPlatformString(accountID),
                                    "" /* device id not used with sip */,
                                    "" /* device name not used with sip */,
                                    active,
                                    false /* upnpn not used with sip */,
                                    false,
                                    true,
                                    false,
                                    "",
                                    Utils::toPlatformString(sipHostname),
                                    Utils::toPlatformString(sipUsername),
                                    Utils::toPlatformString(sipPassword));

    accountsList_->InsertAt(0, account);
    contactListModels_->Insert(account->accountID_, ref new ContactListModel(account->accountID_));
    accountAdded(account);
    return account;
}

void
AccountsViewModel::clearAccountList()
{
    accountsList_->Clear();
    clearAccountsList();
}

Account ^ RingClientUWP::ViewModel::AccountsViewModel::findItem(String ^ accountId)
{
    for each (Account^ item in accountsList_)
        if (item->accountID_ == accountId)
            return item;

    return nullptr;
}

Account^
AccountsViewModel::findAccountByRingID(String ^ ringId)
{
    for each (Account^ item in accountsList_)
        if (item->ringID_ == ringId)
            return item;

    return nullptr;
}

ContactListModel^
AccountsViewModel::getContactListModel(std::string& accountId)
{
    if (contactListModels_->Size)
        return contactListModels_->Lookup(Utils::toPlatformString(accountId));
    return nullptr;
}

int
AccountsViewModel::unreadMessages(String ^ accountId)
{
    int messageCount = 0;
    auto acceptIncognitoMessages = findItem(accountId)->_dhtPublicInCalls;
    if (auto contactListModel = getContactListModel(Utils::toString(accountId))) {
        for each (auto contact in contactListModel->_contactsList) {
            if (acceptIncognitoMessages || contact->_trustStatus == TrustStatus::TRUSTED)
                messageCount += contact->_unreadMessages;
        }
    }
    return messageCount;
}

int
AccountsViewModel::unreadContactRequests(String ^ accountId)
{
    int contactRequestCount = 0;
    if (auto contactListModel = getContactListModel(Utils::toString(accountId))) {
        for each (auto contact in contactListModel->_contactsList) {
            if (contact->_trustStatus == TrustStatus::INCOMING_CONTACT_REQUEST) {
                contactRequestCount += contact->_unreadContactRequest ? 1 : 0;
            }
        }
    }
    return contactRequestCount;
}

int
AccountsViewModel::bannedContacts(String^ accountId)
{
    int bannedContacts = 0;
    if (auto contactListModel = getContactListModel(Utils::toString(accountId))) {
        for each (auto contact in contactListModel->_contactsList) {
            if (contact->_trustStatus == TrustStatus::BLOCKED) {
                bannedContacts++;
            }
        }
    }
    return bannedContacts;
}

int
AccountsViewModel::activeAccounts()
{
    int totalActiveAccounts = 0;
    for (auto account : accountsList_) {
        if (account->_active)
            totalActiveAccounts++;
    }
    return totalActiveAccounts;
}

void
AccountsViewModel::OnincomingAccountMessage(String ^ accountId, String ^ fromRingId, String ^ payload)
{
    auto contactListModel = getContactListModel(Utils::toString(accountId));

    auto contact = contactListModel->findContactByRingId(fromRingId);

    bool itemSelected = false;
    if (auto selectedItem = SmartPanelItemsViewModel::instance->_selectedItem)
        itemSelected = (selectedItem->_contact == contact);
    auto isInBackground = RingD::instance->isInBackground;

    if (contact == nullptr) {
        contact = contactListModel->addNewContact(fromRingId, fromRingId, TrustStatus::UNKNOWN, true);
        RingD::instance->lookUpAddress(Utils::toString(accountId), fromRingId);
        if (!itemSelected || isInBackground || RingD::instance->isInvisible) {
            RingD::instance->unpoppedToasts.insert(std::make_pair(contact->ringID_,
                [isInBackground, fromRingId, payload](String^ username) {
                RingD::instance->ShowIMToast(isInBackground, fromRingId, payload, username);
            }));
        }
    }
    else {
        contact->_isIncognitoContact = true;
        if (!itemSelected || isInBackground || RingD::instance->isInvisible) {
            RingD::instance->ShowIMToast(isInBackground, fromRingId, payload);
        }
    }

    auto messageId = Utils::toPlatformString(Utils::genID(0LL, 9999999999999999999LL));
    contact->_conversation->addMessage(MSG_FROM_CONTACT, payload, std::time(nullptr), false, messageId);

    /* save contacts conversation to disk */
    contact->saveConversationToFile();

    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto selectedContact = (item) ? item->_contact : nullptr;
    newUnreadMessage(contact);
}

void
AccountsViewModel::OnincomingMessage(String ^callId, String ^payload)
{
    auto itemlist = SmartPanelItemsViewModel::instance->itemsList;
    auto item = SmartPanelItemsViewModel::instance->findItem(callId);
    auto contact = item->_contact;
    auto contactListModel = getContactListModel(Utils::toString(contact->_accountIdAssociated));

    /* the contact HAS TO BE already registered */
    if (contact) {
        auto item = SmartPanelItemsViewModel::instance->_selectedItem;

        auto messageId = Utils::toPlatformString(Utils::genID(0LL, 9999999999999999999LL));
        contact->_conversation->addMessage(MSG_FROM_CONTACT, payload, std::time(nullptr), false, messageId);

        /* save contacts conversation to disk */
        contact->saveConversationToFile();

        auto selectedContact = (item) ? item->_contact : nullptr;

        newUnreadMessage(contact);
    }
}