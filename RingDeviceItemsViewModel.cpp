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

#include "pch.h"

#include "RingDeviceItemsViewModel.h"

using namespace Windows::ApplicationModel::Core;
using namespace Windows::Data::Json;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Core;
using namespace Windows::Globalization::DateTimeFormatting;


using namespace RingClientUWP;
using namespace ViewModel;

RingDeviceItemsViewModel::RingDeviceItemsViewModel()
{
    itemsList_ = ref new Vector<RingDeviceItem^>();
}

RingDeviceItem^
RingDeviceItemsViewModel::findItem(String^ deviceId)
{
    for each (RingDeviceItem^ item in itemsList)
        if (item->_deviceId == deviceId)
            return item;

    return nullptr;
}

unsigned int
RingDeviceItemsViewModel::getIndex(String^ deviceId)
{
    for (unsigned i = 0; i < itemsList_->Size; i++)
        if (itemsList_->GetAt(i)->_deviceId == deviceId)
            return i;

    return 0;
}

void
RingDeviceItemsViewModel::removeItem(RingDeviceItem ^ item)
{
    unsigned int index;

    if (itemsList->IndexOf(item, &index)) {
        itemsList->RemoveAt(index);
    }
}