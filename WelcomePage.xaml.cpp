/**************************************************************************
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

#include "WelcomePage.xaml.h"
#include "AboutPage.xaml.h"

using namespace RingClientUWP;
using namespace RingClientUWP::Views;

using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Controls;

WelcomePage::WelcomePage()
{
    InitializeComponent();
};

void RingClientUWP::Views::WelcomePage::_welcomeAboutButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto rootFrame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Window::Current->Content);
    rootFrame->Navigate(Windows::UI::Xaml::Interop::TypeName(Views::AboutPage::typeid));
}
