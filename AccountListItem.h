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

#pragma once

using namespace Platform;
using namespace Windows::Data::Json;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

#include <RingDebug.h>

namespace RingClientUWP
{
namespace Controls
{

public ref class AccountListItem sealed : public INotifyPropertyChanged
{
public:
    AccountListItem(Account^ a);

    void raiseNotifyPropertyChanged(String^ propertyName);
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property Account^ _account;

    property bool _editionMode;
    property bool _disconnected;

    property bool _isSelected {
        bool get() {
            return isSelected_;
        }
        void set(bool value) {
            isSelected_ = value;
            if (!_disconnected)
                NotifyPropertyChanged("_isSelected");
        }
    }

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    bool isSelected_;
};
}
}

