﻿#pragma once
/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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
#include "MainPage.g.h"

using namespace Windows::UI::Xaml::Input;

namespace RingClientUWP
{
namespace Views {
}
public ref class MainPage sealed
{
public:
    MainPage();

protected:
    virtual void OnKeyDown(KeyRoutedEventArgs^ e) override;
private:
    void _toggleSmartBoxButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
};
}