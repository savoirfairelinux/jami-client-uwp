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
#include "pch.h"

#include "SmartPanel.xaml.h"
#include "RingConsolePanel.xaml.h"
#include "WelcomePage.xaml.h"

#include "MainPage.xaml.h"

using namespace RingClientUWP;
using namespace RingClientUWP::Views;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::ViewManagement;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Core;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Graphics::Display;
using namespace Windows::System;

MainPage::MainPage()
{
    InitializeComponent();

    _welcomeframe_->Navigate(TypeName(RingClientUWP::Views::WelcomePage::typeid));
    _smartPanel_->Navigate(TypeName(RingClientUWP::Views::SmartPanel::typeid));
    _consolePanel_->Navigate(TypeName(RingClientUWP::Views::RingConsolePanel::typeid));
}

void
MainPage::OnKeyDown(KeyRoutedEventArgs^ e)
{
    if (e->Key == VirtualKey::F5) {
        _outerSplitView_->OpenPaneLength = Window::Current->Bounds.Width;
        _outerSplitView_->IsPaneOpen = !_outerSplitView_->IsPaneOpen;
    }
}