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
namespace ViewModel {

public ref class SmartPanelItemsViewModel sealed
{
internal:
    /* singleton */
    static property SmartPanelItemsViewModel^ instance
    {
        SmartPanelItemsViewModel^ get()
        {
            static SmartPanelItemsViewModel^ instance_ = ref new SmartPanelItemsViewModel();
            return instance_;
        }
    }

    /* functions */
    SmartPanelItem^ findItem(String^ callId);
    SmartPanelItem^ findItem(Contact^ contact);
    unsigned int getIndex(String^ callId);
    unsigned int getIndex(Contact^ contact);
    void removeItem(SmartPanelItem^ item);

    property Vector<SmartPanelItem^>^ itemsList
    {
        Vector<SmartPanelItem^>^ get()
        {
            return itemsList_;
        }
    }

    property SmartPanelItem^ _selectedItem
    {
        SmartPanelItem^ get()
        {
            return currentItem_;
        }
        void set(SmartPanelItem^ value)
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
    SmartPanelItemsViewModel(); // singleton
    Vector<SmartPanelItem^>^ itemsList_;
    SmartPanelItem^ currentItem_;
    SmartPanelItem^ oldItem_;


};
}
}
