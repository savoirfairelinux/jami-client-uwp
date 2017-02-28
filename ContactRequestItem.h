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

using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Data;

namespace RingClientUWP
{
namespace Controls
{

public ref class ContactRequestItem sealed : public INotifyPropertyChanged
{
public:
    ContactRequestItem();

    void raiseNotifyPropertyChanged(String^ propertyName);
    virtual event PropertyChangedEventHandler^ PropertyChanged;

    property Contact^ _contact {
        Contact^ get() { return contact_; }
        void set(Contact^ value) {
            contact_ = value;
            NotifyPropertyChanged("_contact");
        }
    };

    property Visibility _isVisible
    {
        Visibility get() {
            return isVisible_;
        }
        void set(Visibility value) {
            isVisible_ = value;
            NotifyPropertyChanged("_isVisible");
        }
    }

    property bool _isSelected
    {
        bool get() {
            return isSelected_;
        }
        void set(bool value) {
            isSelected_ = value;
            NotifyPropertyChanged("_isSelected");
        }
    }

    property bool _isHovered
    {
        bool get() {
            return isHovered_;
        }
        void set(bool value) {
            isHovered_ = value;
            NotifyPropertyChanged("_isHovered");
        }
    }

protected:
    void NotifyPropertyChanged(String^ propertyName);

private:
    Visibility isVisible_;
    bool isSelected_;
    bool isHovered_;

    Contact^ contact_;
};
}
}
