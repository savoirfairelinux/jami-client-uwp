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
#pragma once

#include "ContactListModel.h"

using namespace Platform::Collections;

namespace RingClientUWP
{
ref class Contact;

delegate void NewAccountSelected();
delegate void NoAccountSelected();
delegate void UpdateScrollView();
delegate void AccountAdded(Account^ account);
delegate void ClearAccountsList();
delegate void ContactAdded(String^, Contact^);
delegate void ContactDeleted(String^, Contact^);
delegate void ContactDataModified(String^, Contact^);
delegate void NewUnreadMessage();
delegate void NewUnreadContactRequest();

namespace ViewModel
{

public ref class AccountsViewModel sealed
{
public:
    void raiseContactAdded(String^ accountId, Contact^ name);
    void raiseContactDeleted(String^ accountId, Contact^ name);
    void raiseContactDataModified(String^ accountId, Contact^ name);
    void raiseUnreadContactRequest();

internal:
    /* properties */
    static property AccountsViewModel^ instance {
        AccountsViewModel^ get() {
            static AccountsViewModel^ instance_ = ref new AccountsViewModel();
            return instance_;
        }
    }

    property Vector<Account^>^ accountsList {
        Vector<Account^>^ get() {
            return accountsList_;
        }
    }

    /* functions */
    void addRingAccount(std::string& alias, std::string& ringID, std::string& accountID, std::string& deviceId, bool upnpState, bool autoAnswer, bool dhtPublicInCalls);
    void addSipAccount(std::string& alias, std::string& accountID, std::string& sipHostname, std::string& sipUsername, std::string& sipPassword);
    void clearAccountList();
    Account^ findItem(String^ accountId);
    ContactListModel^ getContactListModel(std::string& accountId);
    int unreadMessages(String^ accountId);
    int unreadContactRequests(String^ accountId);

    /* events */
    event NewAccountSelected^ newAccountSelected;
    event NoAccountSelected^ noAccountSelected;
    event UpdateScrollView^ updateScrollView;
    event AccountAdded^ accountAdded;
    event ClearAccountsList^ clearAccountsList;
    event ContactAdded^ contactAdded;
    event ContactDeleted^ contactDeleted;
    event ContactDataModified^ contactDataModified;
    event NewUnreadMessage^ newUnreadMessage;
    event NewUnreadContactRequest^ newUnreadContactRequest;

private:
    AccountsViewModel();

    void OnincomingAccountMessage(String^ accountId, String^ fromRingId, String^ payload);
    void OnincomingMessage(String ^callId, String ^payload);

    Vector<Account^>^ accountsList_;
    Map<String^, ContactListModel^>^ contactListModels_;
};
}
}
