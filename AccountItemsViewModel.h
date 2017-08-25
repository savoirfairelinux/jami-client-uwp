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

#include "AccountItem.h"

using namespace Platform::Collections;
using namespace Concurrency;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;

namespace RingClientUWP
{
namespace ViewModel
{

public ref class AccountItemsViewModel sealed
{

/* singleton */
private:
    AccountItemsViewModel();

internal:
    static property AccountItemsViewModel^ instance {
        AccountItemsViewModel^ get() {
            static AccountItemsViewModel^ instance_ = ref new AccountItemsViewModel();
            return instance_;
        }
    }

internal:
    /* functions */
    String^ getSelectedAccountId();
    void addItem(String^ id, Map<String^, String^>^ details);
    AccountItem^ findItem(String^ accountId);
    void removeItem(AccountItem^ item);
    int getIndex(String^ accountId);

internal:
    /* properties */
    property Vector<AccountItem^>^ _itemsList
    {
        Vector<AccountItem^>^ get() {
            return itemsList_;
        }
    }

    property AccountItem^ _selectedItem
    {
        AccountItem^ get() {
            return currentItem_;
        }
        void set(AccountItem^ value) {
            if (currentItem_)
                currentItem_->_isSelected = false;
            currentItem_ = value;
            // emit signal : accountSelected
        }
    }

private:
    Vector<AccountItem^>^ itemsList_;
    AccountItem^ currentItem_;

};
}
}
