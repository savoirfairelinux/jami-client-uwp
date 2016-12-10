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

#include "SmartPanelItemsViewModel.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;


using namespace RingClientUWP;
using namespace ViewModel;

SmartPanelItemsViewModel::SmartPanelItemsViewModel()
{
    itemsList_ = ref new Vector<SmartPanelItem^>();
}

SmartPanelItem^
SmartPanelItemsViewModel::findItem(String^ callId)
{
    for each (SmartPanelItem^ item in itemsList)
        if (item->_callId == callId)
            return item;

    return nullptr;
}

SmartPanelItem^
SmartPanelItemsViewModel::findItem(Contact^ contact)
{
    for each (SmartPanelItem^ item in itemsList)
        if (item->_contact == contact)
            return item;

    return nullptr;
}

unsigned int
SmartPanelItemsViewModel::getIndex(String^ callId)
{
    unsigned int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_callId == callId)
            break;
    }
    return i;
}

unsigned int
SmartPanelItemsViewModel::getIndex(Contact^ contact)
{
    unsigned int i;
    for (i = 0; i < itemsList_->Size; i++) {
        if (itemsList_->GetAt(i)->_contact == contact)
            break;
    }
    return i;
}

void RingClientUWP::ViewModel::SmartPanelItemsViewModel::removeItem(SmartPanelItem ^ item)
{
    unsigned int index;

    if (itemsList->IndexOf(item, &index))
        itemsList->RemoveAt(index);
}

void RingClientUWP::ViewModel::SmartPanelItemsViewModel::moveItemToTheTop(SmartPanelItem ^ item)
{
    unsigned int index;

    if (itemsList->IndexOf(item, &index)) {
        if (index != 0) {
            itemsList->RemoveAt(index);
            itemsList->InsertAt(0, item);
            item->_isHovered = false;
        }
    }
}
