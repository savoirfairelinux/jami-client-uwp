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

#include "Globals.h"

using namespace Platform::Collections;
using namespace Concurrency;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;

namespace RingClientUWP
{
namespace ViewModel
{

ref class SmartPanelItemsViewModel sealed
{
public:
    bool isInCall();
    String^ getAssociatedAccountId(SmartPanelItem^ item);
    void refreshFilteredItemsList();
    void refreshFilteredBannedItemsList();

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
    void update(const std::vector<std::string>& properties);
    SmartPanelItem^ findItem(String^ callId);
    SmartPanelItem^ findItem(Contact^ contact);
    SmartPanelItem^ findItemByRingID(String^ ringID);

    unsigned int getIndex(String^ callId);
    unsigned int getIndex(Contact^ contact);
    unsigned int getFilteredIndex(Contact^ contact);

    void removeItem(SmartPanelItem^ item);
    void moveItemToTheTop(SmartPanelItem^ item);

    property IObservableVector<SmartPanelItem^>^ itemsList
    {
        IObservableVector<SmartPanelItem^>^ get() {
            return itemsList_;
        }
    }

    property IObservableVector<SmartPanelItem^>^ itemsListFiltered
    {
        IObservableVector<SmartPanelItem^>^ get() {
            return itemsListFiltered_;
        }
        void set(IObservableVector<SmartPanelItem^>^ value) {
            itemsListFiltered_ = dynamic_cast<Vector<SmartPanelItem^>^>(value);
        }
    }

    property IObservableVector<SmartPanelItem^>^ itemsListBannedFiltered
    {
        IObservableVector<SmartPanelItem^>^ get() {
            return itemsListBannedFiltered_;
        }
        void set(IObservableVector<SmartPanelItem^>^ value) {
            itemsListBannedFiltered_ = dynamic_cast<Vector<SmartPanelItem^>^>(value);
        }
    }

    property SmartPanelItem^ _selectedItem
    {
        SmartPanelItem^ get() {
            return currentItem_;
        }
        void set(SmartPanelItem^ value) {
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
    SmartPanelItemsViewModel(); // singleton

    Vector<SmartPanelItem^>^ itemsList_;
    Vector<SmartPanelItem^>^ itemsListFiltered_;
    Vector<SmartPanelItem^>^ itemsListBannedFiltered_;

    SmartPanelItem^ currentItem_;
    SmartPanelItem^ oldItem_;

    void OnstateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code);
};

//////////////////////////////
//
// NEW
//
//////////////////////////////

ref class SmartItemsViewModel sealed
{
    /* singleton */
private:
    SmartItemsViewModel();
internal:
    static property SmartItemsViewModel^ instance {
        SmartItemsViewModel^ get() {
            static SmartItemsViewModel^ instance_ = ref new SmartItemsViewModel();
            return instance_;
        }
    }

    SmartItem^  addItem(Object^ item);

    SmartItem^  findItemByCallId(String^ callId);
    SmartItem^  findItemByRingId(String^ ringId);

    unsigned    getIndex(SmartItem^ item);
    unsigned    getIndexByCallId(String^ callId);
    unsigned    getIndexByRingId(String^ ringId);

    void        removeItem(SmartItem^ item);
    void        moveItemToTheTop(SmartItem^ item);

    bool        isInCall();
    String^     getAssociatedAccountId(SmartItem^ item);
    void        refreshFilteredItemsList();
    void        refreshFilteredBannedItemsList();

    property IObservableVector<SmartItem^>^ _itemsList {
        IObservableVector<SmartItem^>^ get() {
            return itemsList_;
        }
    }

    property IObservableVector<SmartItem^>^ _itemsListFiltered {
        IObservableVector<SmartItem^>^ get() {
            return itemsListFiltered_;
        }
        void set(IObservableVector<SmartItem^>^ value) {
            itemsListFiltered_ = dynamic_cast<Vector<SmartItem^>^>(value);
        }
    }

    property IObservableVector<SmartItem^>^ _itemsListBannedFiltered {
        IObservableVector<SmartItem^>^ get() {
            return itemsListBannedFiltered_;
        }
        void set(IObservableVector<SmartItem^>^ value) {
            itemsListBannedFiltered_ = dynamic_cast<Vector<SmartItem^>^>(value);
        }
    }

    property SmartItem^ _selectedItem {
        SmartItem^ get() {
            return currentItem_;
        }
        void set(SmartItem^ value) {
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
    Vector<SmartItem^>^ itemsList_;
    Vector<SmartItem^>^ itemsListFiltered_;
    Vector<SmartItem^>^ itemsListBannedFiltered_;

    SmartItem^          currentItem_;
    SmartItem^          oldItem_;

};
}
}
