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

using namespace Platform::Collections;
using namespace Concurrency;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;

namespace RingClientUWP
{
namespace ViewModel
{

public ref class AccountListItemsViewModel sealed
{
public:
    String^ getSelectedAccountId();
    int unreadMessages();
    int unreadContactRequests();

internal:
    /* singleton */
    static property AccountListItemsViewModel^ instance
    {
        AccountListItemsViewModel^ get()
        {
            static AccountListItemsViewModel^ instance_ = ref new AccountListItemsViewModel();
            return instance_;
        }
    }

    /* functions */
    void update(const std::vector<std::string>& properties);
    AccountListItem^ findItem(String^ accountId);
    int getIndex(String^ accountId);
    void removeItem(AccountListItem^ item);

    /* properties */
    property Vector<AccountListItem^>^ itemsList
    {
        Vector<AccountListItem^>^ get() {
            return itemsList_;
        }
    }

    property AccountListItem^ _selectedItem
    {
        AccountListItem^ get() {
            return currentItem_;
        }
        void set(AccountListItem^ value) {
            if (currentItem_)
                currentItem_->_isSelected = false;
            currentItem_ = value;
            updateContactsViewModel();
        }
    }

private:
    AccountListItemsViewModel(); // singleton
    Vector<AccountListItem^>^ itemsList_;
    AccountListItem^ currentItem_;

    void OnaccountAdded(RingClientUWP::Account ^account);
    void OnclearAccountsList();
    void updateContactsViewModel();
};
}
}
