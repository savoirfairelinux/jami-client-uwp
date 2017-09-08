/***************************************************************************
* Copyright (C) 2017 by Savoir-faire Linux                                *
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

#include "ConversationItem.h"

using namespace Platform::Collections;
using namespace Concurrency;

using namespace RingClientUWP;
using namespace RingClientUWP::Controls;

namespace RingClientUWP
{

ref class ConversationItemsViewModel sealed
{
    /* singleton */
private:
    ConversationItemsViewModel();
internal:
    static property ConversationItemsViewModel^ instance {
        ConversationItemsViewModel^ get() {
            static ConversationItemsViewModel^ instance_ = ref new ConversationItemsViewModel();
            return instance_;
        }
    }

    ConversationItem^  addItem(Object^ item);

    ConversationItem^  findItemByCallId(String^ callId);
    ConversationItem^  findItemByRingId(String^ ringId);

    unsigned    getIndex(ConversationItem^ item);
    unsigned    getIndexByCallId(String^ callId);
    unsigned    getIndexByRingId(String^ ringId);

    void        removeItem(ConversationItem^ item);
    void        moveItemToTheTop(ConversationItem^ item);

    bool        isInCall();
    String^     getAssociatedAccountId(ConversationItem^ item);
    void        refreshFilteredItemsList();
    void        refreshFilteredBannedItemsList();

    property IObservableVector<ConversationItem^>^ _itemsList {
        IObservableVector<ConversationItem^>^ get() {
            return itemsList_;
        }
    }

    property IObservableVector<ConversationItem^>^ _itemsListFiltered {
        IObservableVector<ConversationItem^>^ get() {
            return itemsListFiltered_;
        }
        void set(IObservableVector<ConversationItem^>^ value) {
            itemsListFiltered_ = dynamic_cast<Vector<ConversationItem^>^>(value);
        }
    }

    property IObservableVector<ConversationItem^>^ _itemsListBannedFiltered {
        IObservableVector<ConversationItem^>^ get() {
            return itemsListBannedFiltered_;
        }
        void set(IObservableVector<ConversationItem^>^ value) {
            itemsListBannedFiltered_ = dynamic_cast<Vector<ConversationItem^>^>(value);
        }
    }

    property ConversationItem^ _selectedItem {
        ConversationItem^ get() {
            return currentItem_;
        }
        void set(ConversationItem^ value) {
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
    Vector<ConversationItem^>^ itemsList_;
    Vector<ConversationItem^>^ itemsListFiltered_;
    Vector<ConversationItem^>^ itemsListBannedFiltered_;

    ConversationItem^          currentItem_;
    ConversationItem^          oldItem_;

};

} /*namespace RingClientUWP*/