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

#include "RingDeviceItem.h"

using namespace Platform::Collections;
using namespace Concurrency;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;

namespace RingClientUWP
{

namespace ViewModel
{

public ref class RingDeviceItemsViewModel sealed
{
internal:
    /* singleton */
    static property RingDeviceItemsViewModel^ instance {
        RingDeviceItemsViewModel^ get() {
            static RingDeviceItemsViewModel^ instance_ = ref new RingDeviceItemsViewModel();
            return instance_;
        }
    }

    /* functions */
    RingDeviceItem^ findItem(String^ deviceId);
    unsigned int    getIndex(String^ deviceId);
    void            removeItem(RingDeviceItem^ item);

    property IObservableVector<RingDeviceItem^>^ itemsList {
        IObservableVector<RingDeviceItem^>^ get() {
            return itemsList_;
        }
    }

    property RingDeviceItem^ _selectedItem {
        RingDeviceItem^ get() {
            return currentItem_;
        }
        void set(RingDeviceItem^ value) {
            oldItem_ = currentItem_;
            currentItem_ = value;

            if (oldItem_ != nullptr)
                oldItem_->_isSelected = false;

            if (currentItem_ != nullptr) {
                currentItem_->_isSelected = true;
            }
        }
    }

private:
    RingDeviceItemsViewModel(); // singleton

    Vector<RingDeviceItem^>^ itemsList_;

    RingDeviceItem^ currentItem_;
    RingDeviceItem^ oldItem_;
};
}
}
