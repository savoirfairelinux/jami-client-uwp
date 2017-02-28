/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
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

#pragma once

#include "ContactRequestItem.h"

using namespace Platform::Collections;
using namespace Concurrency;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;

namespace RingClientUWP
{
namespace ViewModel {

public ref class ContactRequestItemsViewModel sealed
{
internal:
    /* singleton */
    static property ContactRequestItemsViewModel^ instance
    {
        ContactRequestItemsViewModel^ get()
        {
            static ContactRequestItemsViewModel^ instance_ = ref new ContactRequestItemsViewModel();
            return instance_;
        }
    }

    /* functions */
    ContactRequestItem^ findItem(String^ callId);
    ContactRequestItem^ findItem(Contact^ contact);
    unsigned int getIndex(String^ callId);
    unsigned int getIndex(Contact^ contact);
    void removeItem(ContactRequestItem^ item);
    void moveItemToTheTop(ContactRequestItem^ item);

    property Vector<ContactRequestItem^>^ itemsList
    {
        Vector<ContactRequestItem^>^ get()
        {
            return itemsList_;
        }
    }

    property ContactRequestItem^ _selectedItem
    {
        ContactRequestItem^ get()
        {
            return currentItem_;
        }
        void set(ContactRequestItem^ value)
        {
            oldItem_ = currentItem_;
            currentItem_ = value;

            if (oldItem_ != nullptr)
                oldItem_->_isSelected = false;

            if (currentItem_ != nullptr)
                currentItem_->_isSelected = true;
        }
    }

private:
    ContactRequestItemsViewModel(); // singleton
    Vector<ContactRequestItem^>^ itemsList_;
    ContactRequestItem^ currentItem_;
    ContactRequestItem^ oldItem_;
    void OnstateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code);
};
}
}
