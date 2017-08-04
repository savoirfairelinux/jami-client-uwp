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
#pragma once

#include "MainPage.g.h"
#include "SmartPanelItem.h"

using namespace Windows::UI::Xaml::Input;
using namespace Windows::Foundation;
using namespace Windows::ApplicationModel::ExtendedExecution;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Networking::Connectivity;

namespace RingClientUWP
{

enum class FrameOpen {
    WELCOME,
    MESSAGE,
    VIDEO
};

public ref class MainPage sealed
{
public:
    MainPage();

    void showLoadingOverlay(bool load, bool modal);
    void hideLoadingOverlay();
    void OnsetOverlayStatusText(String^ statusText, String^ color);
    void focusOnMessagingTextbox();
    void updateMessageTextPage(SmartPanelItem^ item);

    property bool isLoading;
    property bool isModal;
    property bool isSmartPanelOpen;

internal:
    property FrameOpen currentFrame {
        FrameOpen get() {
            return _currentFrame;
        }
    };

protected:
    virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
    virtual void OnKeyDown(KeyRoutedEventArgs^ e) override;

    void OnResize(Platform::Object^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ e);

private:
    // Visibility and suspension
    void Application_Suspending(Object^, Windows::ApplicationModel::SuspendingEventArgs^ e);
    EventRegistrationToken applicationSuspendingEventToken;
    void Application_VisibilityChanged(Object^ sender, Windows::UI::Core::VisibilityChangedEventArgs^ e);
    EventRegistrationToken visibilityChangedEventToken;
    void Application_Resuming(Object^ sender, Object^ args);
    EventRegistrationToken applicationResumingEventToken;

    // Multi-monitor, DPI, scale factor change, and window resize detection
    void DisplayProperties_DpiChanged(Windows::Graphics::Display::DisplayInformation^ sender, Platform::Object^ args);
    EventRegistrationToken dpiChangedtoken;
    Rect bounds;
    FrameOpen _currentFrame;
    bool editionMode = false;

    void _toggleSmartBoxButton__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void setSmartPanelState(bool open);
    void showFrame(Windows::UI::Xaml::Controls::Frame^ frame);
    void OnsummonMessageTextPage();
    void OnFullScreenToggled(bool state);
    void OnsummonWelcomePage();
    void OnsummonPreviewPage();
    void OnhidePreviewPage();
    void OnsummonVideoPage();
    void OnpressHangUpCall();
    void OnstateChange(Platform::String ^callId, CallStatus state, int code);
    void OncloseMessageTextPage();
    void OnregistrationStateErrorGeneric(const std::string& accountId);
    void OnregistrationStateUnregistered(const std::string& accountId);
    void OnregistrationStateRegistered(const std::string& accountId);
    void OncallPlaced(Platform::String ^callId);
    void OnnameRegistred(bool status, String ^accountId);
    void OnvolatileDetailsChanged(const std::string &accountId, const std::map<std::string, std::string>& details);
};
}
