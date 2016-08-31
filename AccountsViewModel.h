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

#pragma once

using namespace Platform::Collections;

namespace RingClientUWP
{

delegate void NewAccountSelected();
delegate void NoAccountSelected();

namespace ViewModel {
public ref class AccountsViewModel sealed
{
internal:
    /* singleton */
    static property AccountsViewModel^ instance
    {
        AccountsViewModel^ get()
        {
            static AccountsViewModel^ instance_ = ref new AccountsViewModel();
            return instance_;
        }
    }

    /* functions */
    void add(std::string& name, std::string& ringID, std::string& accountType, std::string& accountID);
    void clearAccountList();

    /* properties */
    property Account^ selectedAccount
    {
        Account^ get()
        {
            return currentItem_;
        }
        void set(Account^ value)
        {
            oldItem_ = currentItem_;
            currentItem_ = value;
            if (value)
                newAccountSelected();
            else
                noAccountSelected();
        }
    }

    property Vector<Account^>^ accountsList
    {
        Vector<Account^>^ get()
        {
            return accountsList_;
        }
    }

    /* events */
    event NewAccountSelected^ newAccountSelected;
    event NoAccountSelected^ noAccountSelected;

private:
    AccountsViewModel(); // singleton
    Vector<Account^>^ accountsList_;
    Account^ currentItem_;
    Account^ oldItem_;

};
}
}
