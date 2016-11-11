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

using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::ExtendedExecution;

namespace RingClientUWP
{

namespace Views {
}
public ref class MainPage sealed
{
public:
    MainPage();
    void showLoadingOverlay(bool load, bool modal);
    void hideLoadingOverlay();

    property bool isLoading;

protected:
    virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
    virtual void OnKeyDown(KeyRoutedEventArgs^ e) override;

    void PositionImage();
    void PositionRing();
    void OnResize(Platform::Object^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ e);

private:
    // Visibility and suspension
    void Application_Suspending(Object^, Windows::ApplicationModel::SuspendingEventArgs^ e);
    EventRegistrationToken applicationSuspendingEventToken;
    void Application_VisibilityChanged(Object^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ e);
    EventRegistrationToken visibilityChangedEventToken;
    void Application_Resuming(Object^ sender, Object^ args);
    EventRegistrationToken applicationResumingEventToken;
    //void Application_Closing(Object^ sender, Windows::UI::Core::^ e);
    //EventRegistrationToken applicationClosingEventToken;

    ExtendedExecutionSession^ session;
    void SessionRevoked(Object^ sender, ExtendedExecutionRevokedEventArgs^ args);
    EventRegistrationToken sessionRevokedToken;
    task<void> BeginExtendedExecution();
    void ClearExtendedExecution();

    // Multi-monitor, DPI, scale factor change, and window resize detection
    void DisplayProperties_DpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
    EventRegistrationToken dpiChangedtoken;
    Rect bounds;

    void _toggleSmartBoxButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void showFrame(Windows::UI::Xaml::Controls::Frame^ frame);
    void OnsummonMessageTextPage();
    void OnsummonWelcomePage();
    void OnsummonPreviewPage();
    void OnhidePreviewPage();
    void OnsummonVideoPage();
    void OnpressHangUpCall();
    void OnstateChange(Platform::String ^callId, CallStatus state, int code);
    void OncloseMessageTextPage();
};
}
