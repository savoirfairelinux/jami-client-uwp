﻿/**************************************************************************
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

    Window::Current->SizeChanged += ref new WindowSizeChangedEventHandler(this, &MainPage::OnResize);

    _welcomeFrame_->Navigate(TypeName(RingClientUWP::Views::WelcomePage::typeid));
    _smartPanel_->Navigate(TypeName(RingClientUWP::Views::SmartPanel::typeid));
    _consolePanel_->Navigate(TypeName(RingClientUWP::Views::RingConsolePanel::typeid));
    _videoFrame_->Navigate(TypeName(RingClientUWP::Views::VideoPage::typeid));
    _messageTextFrame_->Navigate(TypeName(RingClientUWP::Views::MessageTextPage::typeid));

    /* connect to delegates */
    RingD::instance->stateChange += ref new RingClientUWP::StateChange(this, &RingClientUWP::MainPage::OnstateChange);
    auto smartPanel = dynamic_cast<SmartPanel^>(_smartPanel_->Content);
    smartPanel->summonMessageTextPage += ref new RingClientUWP::SummonMessageTextPage(this, &RingClientUWP::MainPage::OnsummonMessageTextPage);
    smartPanel->summonWelcomePage += ref new RingClientUWP::SummonWelcomePage(this, &RingClientUWP::MainPage::OnsummonWelcomePage);
    smartPanel->summonVideoPage += ref new RingClientUWP::SummonVideoPage(this, &RingClientUWP::MainPage::OnsummonVideoPage);
    auto videoPage = dynamic_cast<VideoPage^>(_videoFrame_->Content);
    videoPage->pressHangUpCall += ref new RingClientUWP::PressHangUpCall(this, &RingClientUWP::MainPage::OnpressHangUpCall);

    DisplayInformation^ displayInformation = DisplayInformation::GetForCurrentView();
    dpiChangedtoken = (displayInformation->DpiChanged += ref new TypedEventHandler<DisplayInformation^,
                       Platform::Object^>(this, &MainPage::DisplayProperties_DpiChanged));
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
    }
}

void
RingClientUWP::MainPage::OnNavigatedTo(NavigationEventArgs ^ e)
{
    RingD::instance->startDaemon();
    showLoadingOverlay(true, false);
}

void
RingClientUWP::MainPage::showLoadingOverlay(bool load, bool modal)
{
    if (!isLoading && load) {
        isLoading = true;
        _loadingOverlay_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        if (modal) {
            _fadeInModalStoryboard_->Begin();
            auto blackBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::Black);
            _loadingOverlayRect_->Fill = blackBrush;
        }
        else {
            auto whiteBrush = ref new Windows::UI::Xaml::Media::SolidColorBrush(Windows::UI::Colors::White);
            _loadingOverlayRect_->Fill = whiteBrush;
            _loadingOverlayRect_->Opacity = 1.0;
        }
        OnResize(nullptr, nullptr);
    }
    else if (!load) {
        isLoading = false;
        _fadeOutStoryboard_->Begin();
    }
}

void
RingClientUWP::MainPage::PositionImage()
{
    bounds = ApplicationView::GetForCurrentView()->VisibleBounds;

    auto img = ref new Image();
    auto bitmapImage = ref new Windows::UI::Xaml::Media::Imaging::BitmapImage();
    Windows::Foundation::Uri^ uri;

    if (bounds.Width < 1200)
        uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/TESTS/logo-ring.scale-200.png");
    else
        uri = ref new Windows::Foundation::Uri("ms-appx:///Assets/TESTS/logo-ring.scale-150.png");

    bitmapImage->UriSource = uri;
    img->Source = bitmapImage;
    _loadingImage_->Source = img->Source;

    _loadingImage_->SetValue(Canvas::LeftProperty, bounds.Width * 0.5 - _loadingImage_->Width * 0.5);
    _loadingImage_->SetValue(Canvas::TopProperty, bounds.Height * 0.5 - _loadingImage_->Height * 0.5);
}

void
RingClientUWP::MainPage::PositionRing()
{
    double left;
    double top;
    if (bounds.Width < 1200) {
        _splashProgressRing_->Width = 118;
        _splashProgressRing_->Height = 118;
        left = bounds.Width * 0.5 - _loadingImage_->Width * 0.5 - 145;
        top = bounds.Height * 0.5 - _loadingImage_->Height * 0.5 - 60;
    }
    else {
        _splashProgressRing_->Width = 162;
        _splashProgressRing_->Height = 162;
        left = bounds.Width * 0.5 - _loadingImage_->Width * 0.5 - 195;
        top = bounds.Height * 0.5 - _loadingImage_->Height * 0.5 - 84;
    }
    _splashProgressRing_->SetValue(Canvas::LeftProperty, left + _loadingImage_->Width * 0.5);
    _splashProgressRing_->SetValue(Canvas::TopProperty, top + _loadingImage_->Height * 0.5);
}

void
RingClientUWP::MainPage::OnResize(Platform::Object^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ e)
{
    PositionImage();
    PositionRing();
}

void
RingClientUWP::MainPage::DisplayProperties_DpiChanged(DisplayInformation^ sender, Platform::Object^ args)
{
    OnResize(nullptr, nullptr);
}

void
RingClientUWP::MainPage::hideLoadingOverlay()
{
    _loadingOverlay_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}

void RingClientUWP::MainPage::OnsummonMessageTextPage()
{
    auto messageTextPage = dynamic_cast<MessageTextPage^>(_messageTextFrame_->Content);
    messageTextPage->updatePageContent();
    showFrame(_messageTextFrame_);

}


void RingClientUWP::MainPage::OnsummonWelcomePage()
{
    showFrame(_welcomeFrame_);
}


void RingClientUWP::MainPage::OnsummonVideoPage()
{
    auto videoPage = dynamic_cast<VideoPage^>(_videoFrame_->Content);
    videoPage->updatePageContent();
    showFrame(_videoFrame_);
}


void RingClientUWP::MainPage::OnpressHangUpCall()
{
    OnsummonMessageTextPage();
}



void RingClientUWP::MainPage::OnstateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code)
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;

    switch (state) {
    /* send the user to the peer's message text page */
    case CallStatus::ENDED:
    {
        if (item)
            OnsummonMessageTextPage();
        break;
    }
    /* if the state changes to IN_PROGRESS for any peer, show the video page.
       nb : the peer is currently selected from the SmartPannel. */
    case CallStatus::IN_PROGRESS:
    {
        if (item)
            OnsummonVideoPage();
        break;
    }
    default:
        break;
    }

}

void
MainPage::Application_Suspending(Object^, Windows::ApplicationModel::SuspendingEventArgs^ e)
{
    WriteLine("Application_Suspending");
    if (Frame->CurrentSourcePageType.Name ==
        Interop::TypeName(MainPage::typeid).Name)
    {
        if (Video::VideoManager::instance->captureManager()->captureTaskTokenSource)
            Video::VideoManager::instance->captureManager()->captureTaskTokenSource->cancel();
        //displayInformation->OrientationChanged -= displayInformationEventToken;
        auto deferral = e->SuspendingOperation->GetDeferral();
        Video::VideoManager::instance->captureManager()->CleanupCameraAsync()
            .then([this, deferral]() {
            deferral->Complete();
        });
    }
}

void
MainPage::Application_VisibilityChanged(Object^ sender, VisibilityChangedEventArgs^ e)
{
    if (e->Visible)
    {
        WriteLine("->Visible");
        if (Video::VideoManager::instance->captureManager()->isInitialized) {
            Video::VideoManager::instance->captureManager()->InitializeCameraAsync();
        }
    }
    else
    {
        WriteLine("->Invisible");
    }
}
