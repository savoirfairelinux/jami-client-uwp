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

#include "AccountListItemsViewModel.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;


using namespace RingClientUWP;
using namespace ViewModel;

AccountListItemsViewModel::AccountListItemsViewModel()
{
    itemsList_ = ref new Vector<AccountListItem^>();

    /* connect to delegates */
    AccountsViewModel::instance->accountAdded += ref new RingClientUWP::AccountAdded(this, &RingClientUWP::ViewModel::AccountListItemsViewModel::OnaccountAdded);
    AccountsViewModel::instance->clearAccountsList += ref new RingClientUWP::ClearAccountsList(this, &RingClientUWP::ViewModel::AccountListItemsViewModel::OnclearAccountsList);
}

void RingClientUWP::ViewModel::AccountListItemsViewModel::OnaccountAdded(RingClientUWP::Account ^account)
{
    auto item = ref new AccountListItem(account);
    itemsList_->Append(item);
}


void RingClientUWP::ViewModel::AccountListItemsViewModel::OnclearAccountsList()
{
    itemsList_->Clear();
}
