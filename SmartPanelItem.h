#pragma once
/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

namespace RingClientUWP
{
public ref class SmartPanelItem sealed : public INotifyPropertyChanged
{
public:
    SmartPanelItem();

    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property Contact^ _contact;
    property Call^ _call;

protected:
    void NotifyPropertyChanged(String^ propertyName);
};
}

