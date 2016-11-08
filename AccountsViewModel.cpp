/***************************************************************************
 * Copyright (C) 2016 by Savoir-faire Linux                                *
 * Author: J�ger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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

AccountsViewModel::AccountsViewModel()
{
    accountsList_ = ref new Vector<Account^>();
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
