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
AccountsViewModel::raiseContactAdded(String^ accountId, Contact ^ name)
{
    contactAdded(accountId, name);
}

void
AccountsViewModel::raiseContactDeleted(String^ accountId, Contact ^ name)
{
    contactDeleted(accountId, name);
}

void
AccountsViewModel::raiseContactDataModified(String^ accountId, Contact ^ name)
{
    contactDataModified(accountId, name);
}

void
AccountsViewModel::addRingAccount(std::string& alias, std::string& ringID, std::string& accountID, std::string& deviceId, bool upnpState)
{
    auto account = ref new Account(
                       Utils::toPlatformString(alias),
                       Utils::toPlatformString(ringID),
                       "RING",
                       Utils::toPlatformString(accountID),
                       Utils::toPlatformString(deviceId),
                       upnpState,
                       "" /* sip hostame not used woth ring account */,
                       "" /* sip username not used with ring account */,
                       "" /* sip password not used with ring */ );

    accountsList_->Append(account);
    contactListModels_->Insert(account->accountID_, ref new ContactListModel(account->accountID_));
    updateScrollView();
    accountAdded(account);
}

void
AccountsViewModel::addSipAccount(std::string& alias, std::string& accountID, std::string& sipHostname, std::string& sipUsername, std::string& sipPassword)
{
    auto account = ref new Account(
                       Utils::toPlatformString(alias),
                       "" /* ring Id not used with sip */ ,
                       "SIP",
                       Utils::toPlatformString(accountID),
                       "" /* device id not used with sip */,
                       false /* upnpn not used with sip */,
                       Utils::toPlatformString(sipHostname),
                       Utils::toPlatformString(sipUsername),
                       Utils::toPlatformString(sipPassword)
                   );

    accountsList_->Append(account);
    updateScrollView();
    accountAdded(account);
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

ContactListModel^
AccountsViewModel::getContactListModel(std::string& accountId)
{
    try {
        return contactListModels_->Lookup(Utils::toPlatformString(accountId));
    }
    catch (Platform::Exception^ e) {
        EXC_(e);
        return nullptr;
    }
}

int
AccountsViewModel::unreadMessages(String ^ accountId)
{
    int messageCount = 0;
    if (auto contactListModel = getContactListModel(Utils::toString(accountId))) {
        for each (auto contact in contactListModel->_contactsList) {
            messageCount += contact->_unreadMessages;
        }
    }
    return messageCount;
}

void
AccountsViewModel::OnincomingAccountMessage(String ^ accountId, String ^ fromRingId, String ^ payload)
{
    auto contactListModel = getContactListModel(Utils::toString(accountId));

    auto contact = contactListModel->findContactByRingId(fromRingId);

    if (contact == nullptr)
        contact = contactListModel->addNewContact(fromRingId, fromRingId);

    auto item = SmartPanelItemsViewModel::instance->_selectedItem;

    if (contact == nullptr) {
        ERR_("contact not handled!");
        return;
    }

    RingD::instance->lookUpAddress(fromRingId);

    contact->_conversation->addMessage(""/* date not yet used*/, MSG_FROM_CONTACT, payload);

    /* save contacts conversation to disk */
    contact->saveConversationToFile();

    auto selectedContact = (item) ? item->_contact : nullptr;

    if (contact->ringID_ == fromRingId && contact != selectedContact) {
        contact->_unreadMessages++;
        newUnreadMessage();
        /* saveContactsToFile used to save the notification */
        contactListModel->saveContactsToFile();
    }
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

        contact->_conversation->addMessage(""/* date not yet used*/, MSG_FROM_CONTACT, payload);

        /* save contacts conversation to disk */
        contact->saveConversationToFile();

        auto selectedContact = (item) ? item->_contact : nullptr;

        if (contact != selectedContact) {
            contact->_unreadMessages++;
            newUnreadMessage();
            /* saveContactsToFile used to save the notification */
            contactListModel->saveContactsToFile();
        }
    }
}