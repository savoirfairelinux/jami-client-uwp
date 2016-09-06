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

#include "ContactsViewModel.h"
#include "MessageTextPage.xaml.h"
#include "SmartPanel.xaml.h"
#include "RingConsolePanel.xaml.h"
#include "VideoPage.xaml.h"
#include "WelcomePage.xaml.h"

#include "MainPage.xaml.h"

using namespace RingClientUWP;
using namespace RingClientUWP::Views;
using namespace RingClientUWP::ViewModel;

using namespace Platform;
using namespace Windows::ApplicationModel::Core;
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
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Graphics::Display;
using namespace Windows::System;

MainPage::MainPage()
{
    InitializeComponent();

    _welcomeFrame_->Navigate(TypeName(RingClientUWP::Views::WelcomePage::typeid));
    _smartPanel_->Navigate(TypeName(RingClientUWP::Views::SmartPanel::typeid));
    _consolePanel_->Navigate(TypeName(RingClientUWP::Views::RingConsolePanel::typeid));
    _videoFrame_->Navigate(TypeName(RingClientUWP::Views::VideoPage::typeid));
    _messageTextFrame_->Navigate(TypeName(RingClientUWP::Views::MessageTextPage::typeid));

    /* connect to delegates */
    ContactsViewModel::instance->newContactSelected += ref new NewContactSelected([&]() {
        showFrame(_messageTextFrame_);
    });
    ContactsViewModel::instance->noContactSelected += ref new NoContactSelected([&]() {
        showFrame(_welcomeFrame_);
    });
}

void
MainPage::OnKeyDown(KeyRoutedEventArgs^ e)
{
    if (e->Key == VirtualKey::F5) {
        _outerSplitView_->OpenPaneLength = Window::Current->Bounds.Width;
        _outerSplitView_->IsPaneOpen = !_outerSplitView_->IsPaneOpen;
    }
}

void RingClientUWP::MainPage::_toggleSmartBoxButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    _innerSplitView_->IsPaneOpen = !_innerSplitView_->IsPaneOpen;
    if (_innerSplitView_->IsPaneOpen) {
        dynamic_cast<SmartPanel^>(_smartPanel_->Content)->setMode(Views::SmartPanel::Mode::Normal);
        _hamburgerButtonBar_->Width = 320;
    }
    else {
        dynamic_cast<SmartPanel^>(_smartPanel_->Content)->setMode(Views::SmartPanel::Mode::Minimized);
        _hamburgerButtonBar_->Width = 60;
    }
}

void
RingClientUWP::MainPage::showFrame(Windows::UI::Xaml::Controls::Frame^ frame)
{
    _navGrid_->SetRow(_welcomeFrame_, 0);
    _navGrid_->SetRow(_messageTextFrame_, 0);
    _navGrid_->SetRow(_videoFrame_, 0);

    if (frame == _welcomeFrame_) {
        _navGrid_->SetRow(_welcomeFrame_, 1);
    } else if (frame == _videoFrame_) {
        _navGrid_->SetRow(_videoFrame_, 1);
    } else if (frame == _messageTextFrame_) {
        _navGrid_->SetRow(_messageTextFrame_, 1);
        dynamic_cast<MessageTextPage^>(_messageTextFrame_->Content)->updatePageContent();
    }
}

void
RingClientUWP::MainPage::OnNavigatedTo(NavigationEventArgs ^ e)
{
    RingD::instance->startDaemon();
}