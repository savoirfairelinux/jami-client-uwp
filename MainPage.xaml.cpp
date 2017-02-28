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

#include "ContactListModel.h"
#include "MessageTextPage.xaml.h"
#include "SmartPanel.xaml.h"
//#include "RingConsolePanel.xaml.h"
#include "VideoPage.xaml.h"
#include "PreviewPage.xaml.h"
#include "WelcomePage.xaml.h"
#include "AboutPage.xaml.h"

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
using namespace Concurrency;
using namespace Windows::System::Threading;

MainPage::MainPage()
{
    InitializeComponent();

    UserModel::instance->getUserData();

    Window::Current->SizeChanged += ref new WindowSizeChangedEventHandler(this, &MainPage::OnResize);

    _welcomeFrame_->Navigate(TypeName(RingClientUWP::Views::WelcomePage::typeid));
    _smartPanel_->Navigate(TypeName(RingClientUWP::Views::SmartPanel::typeid));
    //_consolePanel_->Navigate(TypeName(RingClientUWP::Views::RingConsolePanel::typeid));
    _videoFrame_->Navigate(TypeName(RingClientUWP::Views::VideoPage::typeid));
    _previewFrame_->Navigate(TypeName(RingClientUWP::Views::PreviewPage::typeid));
    _messageTextFrame_->Navigate(TypeName(RingClientUWP::Views::MessageTextPage::typeid));

    /* connect to delegates */
    RingD::instance->stateChange += ref new RingClientUWP::StateChange(this, &RingClientUWP::MainPage::OnstateChange);
    auto smartPanel = dynamic_cast<SmartPanel^>(_smartPanel_->Content);
    smartPanel->summonMessageTextPage += ref new RingClientUWP::SummonMessageTextPage(this, &RingClientUWP::MainPage::OnsummonMessageTextPage);
    smartPanel->summonWelcomePage += ref new RingClientUWP::SummonWelcomePage(this, &RingClientUWP::MainPage::OnsummonWelcomePage);
    smartPanel->summonPreviewPage += ref new RingClientUWP::SummonPreviewPage(this, &RingClientUWP::MainPage::OnsummonPreviewPage);
    smartPanel->hidePreviewPage += ref new RingClientUWP::HidePreviewPage(this, &RingClientUWP::MainPage::OnhidePreviewPage);
    smartPanel->summonVideoPage += ref new RingClientUWP::SummonVideoPage(this, &RingClientUWP::MainPage::OnsummonVideoPage);

    auto videoPage = dynamic_cast<VideoPage^>(_videoFrame_->Content);
    videoPage->pressHangUpCall += ref new RingClientUWP::PressHangUpCall(this, &RingClientUWP::MainPage::OnpressHangUpCall);

    auto messageTextFrame = dynamic_cast<MessageTextPage^>(_messageTextFrame_->Content);
    messageTextFrame->closeMessageTextPage += ref new RingClientUWP::CloseMessageTextPage(this, &RingClientUWP::MainPage::OncloseMessageTextPage);

    dpiChangedtoken = (DisplayInformation::GetForCurrentView()->DpiChanged += ref new TypedEventHandler<DisplayInformation^,
                       Platform::Object^>(this, &MainPage::DisplayProperties_DpiChanged));

    visibilityChangedEventToken = Window::Current->VisibilityChanged +=
                                      ref new WindowVisibilityChangedEventHandler(this, &MainPage::Application_VisibilityChanged);

    applicationSuspendingEventToken = Application::Current->Suspending +=
                                          ref new SuspendingEventHandler(this, &MainPage::Application_Suspending);
    applicationResumingEventToken = Application::Current->Resuming +=
                                        ref new EventHandler<Object^>(this, &MainPage::Application_Resuming);

    RingD::instance->registrationStateErrorGeneric += ref new RingClientUWP::RegistrationStateErrorGeneric(this, &RingClientUWP::MainPage::OnregistrationStateErrorGeneric);
    RingD::instance->registrationStateRegistered += ref new RingClientUWP::RegistrationStateRegistered(this, &RingClientUWP::MainPage::OnregistrationStateRegistered);
    RingD::instance->callPlaced += ref new RingClientUWP::CallPlaced(this, &RingClientUWP::MainPage::OncallPlaced);

    RingD::instance->setLoadingStatusText += ref new SetLoadingStatusText([this](String^ statusText, String^ color) {
        _loadingStatus_->Text = statusText;
        auto col = Utils::ColorFromString(color);
        auto brush = ref new Windows::UI::Xaml::Media::SolidColorBrush(col);
        _loadingStatus_->Foreground = brush;
    });

    RingD::instance->fullScreenToggled += ref new RingClientUWP::FullScreenToggled(this, &RingClientUWP::MainPage::OnFullScreenToggled);

}

void
MainPage::focusOnMessagingTextbox()
{
    auto messageTextPage = dynamic_cast<MessageTextPage^>(_messageTextFrame_->Content);
    auto messageTextBox = dynamic_cast<TextBox^>(messageTextPage->FindName("_messageTextBox_"));
    messageTextBox->Focus(Windows::UI::Xaml::FocusState::Programmatic);
}

void
MainPage::OnKeyDown(KeyRoutedEventArgs^ e)
{
    if (DEBUG_ON)
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
        _currentFrame = FrameOpen::WELCOME;
        _navGrid_->SetRow(_welcomeFrame_, 1);
    } else if (frame == _videoFrame_) {
        _currentFrame = FrameOpen::VIDEO;
        _navGrid_->SetRow(_videoFrame_, 1);
    } else if (frame == _messageTextFrame_) {
        _currentFrame = FrameOpen::MESSAGE;
        _navGrid_->SetRow(_messageTextFrame_, 1);
    }

}

void
RingClientUWP::MainPage::OnNavigatedTo(NavigationEventArgs ^ e)
{
    bool fromAboutPage = (e->Parameter != nullptr) ? safe_cast<bool>(e->Parameter) : false;
    if (!fromAboutPage) {
        RingD::instance->init();
        showLoadingOverlay(true, false);
    }
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
    }
    else if (!load && isLoading) {
        isLoading = false;
        _fadeOutStoryboard_->Begin();
    }
}

void
RingClientUWP::MainPage::OnResize(Platform::Object^ sender, Windows::UI::Core::WindowSizeChangedEventArgs^ e)
{
    RingD::instance->raiseWindowResized();
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

void
RingClientUWP::MainPage::OnsummonMessageTextPage()
{
    preloadMessageTextPage(nullptr);
    showFrame(_messageTextFrame_);
}

void
RingClientUWP::MainPage::preloadMessageTextPage(SmartPanelItem^ item)
{
    auto messageTextPage = dynamic_cast<MessageTextPage^>(_messageTextFrame_->Content);
    messageTextPage->updatePageContent(item);
}

void RingClientUWP::MainPage::OnsummonWelcomePage()
{
    showFrame(_welcomeFrame_);
}

void RingClientUWP::MainPage::OnsummonPreviewPage()
{
    MSG_("Show Settings Preview");
    _previewFrame_->Visibility = VIS::Visible;
}

void RingClientUWP::MainPage::OnhidePreviewPage()
{
    MSG_("Hide Settings Preview");
    _previewFrame_->Visibility = VIS::Collapsed;
}

void RingClientUWP::MainPage::OnsummonVideoPage()
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto videoPage = dynamic_cast<VideoPage^>(_videoFrame_->Content);;

    if (item) {
        switch (item->_callStatus) {
        case CallStatus::IN_PROGRESS:
            videoPage->screenVideo(true);
            break;
        case CallStatus::PEER_PAUSED:
        case CallStatus::PAUSED:
            videoPage->screenVideo(false);
            break;
        }
    }

    videoPage->updatePageContent();
    showFrame(_videoFrame_);
}


void RingClientUWP::MainPage::OnpressHangUpCall()
{
    OnsummonMessageTextPage();
}

void
MainPage::OnstateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code)
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;

    switch (state) {
    /* send the user to the peer's message text page */
    case CallStatus::ENDED:
    {
        auto selectedItem = SmartPanelItemsViewModel::instance->_selectedItem;

        if (!selectedItem) {
            return;
        }

        if (item && selectedItem->_callId == callId) {
            OnsummonMessageTextPage();
        }
        break;
    }
    default:
        break;
    }

}

void
MainPage::Application_Suspending(Object^, Windows::ApplicationModel::SuspendingEventArgs^ e)
{
    /*MSG_("Application_Suspending");
    auto deferral = e->SuspendingOperation->GetDeferral();
    Video::VideoManager::instance->captureManager()->CleanupCameraAsync();
    MSG_("Hang up calls...");
    RingD::instance->deinit();
    deferral->Complete();*/
}

void
MainPage::Application_VisibilityChanged(Object^ sender, VisibilityChangedEventArgs^ e)
{
    auto vcm = Video::VideoManager::instance->captureManager();
    if (e->Visible) {
        MSG_("->Visible");
        bool isInCall = false;
        for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
            if (item->_callId && item->_callStatus == CallStatus::IN_PROGRESS) {
                isInCall = true;
                break;
            }
        }
        if (isInCall) {
            vcm->InitializeCameraAsync(false);
            vcm->videoFrameCopyInvoker->Start();
        }
        else if (vcm->isSettingsPreviewing) {
            vcm->CleanupCameraAsync()
            .then([=](task<void> cleanupCameraTask) {
                try {
                    cleanupCameraTask.get();
                    CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(
                        CoreDispatcherPriority::High, ref new DispatchedHandler([=]()
                    {
                        vcm->InitializeCameraAsync(true);
                    }));
                }
                catch (Exception^ e) {
                    WriteException(e);
                }
            });
        }
    }
    else {
        MSG_("->Invisible");
        bool isInCall = false;
        for (auto item : SmartPanelItemsViewModel::instance->itemsList) {
            if (item->_callId && item->_callStatus == CallStatus::IN_PROGRESS) {
                isInCall = true;
                RingD::instance->currentCallId = item->_callId;
                break;
            }
        }
        if (isInCall) {
            if (vcm->isPreviewing) {
                vcm->StopPreviewAsync();
                if (vcm->captureTaskTokenSource)
                    vcm->captureTaskTokenSource->cancel();
                vcm->videoFrameCopyInvoker->Stop();
            }
        }
        else if (vcm->isSettingsPreviewing) {
            vcm->StopPreviewAsync();
        }
    }
}

void MainPage::Application_Resuming(Object^, Object^)
{
    MSG_("Application_Resuming");
}

void RingClientUWP::MainPage::OncloseMessageTextPage()
{
    SmartPanelItemsViewModel::instance->_selectedItem = nullptr;
    showFrame(_welcomeFrame_);
}


void RingClientUWP::MainPage::OnregistrationStateErrorGeneric(const std::string& accountId)
{
    showLoadingOverlay(false, false);
}


void RingClientUWP::MainPage::OnregistrationStateRegistered(const std::string& accountId)
{
    //showLoadingOverlay(false, false);

    /* do not connect those delegates before initial registration on dht is fine.
       Otherwise your going to mess with the wizard */
    RingD::instance->nameRegistred += ref new RingClientUWP::NameRegistred(this, &RingClientUWP::MainPage::OnnameRegistred);
    RingD::instance->volatileDetailsChanged += ref new RingClientUWP::VolatileDetailsChanged(this, &MainPage::OnvolatileDetailsChanged);
}


void RingClientUWP::MainPage::OncallPlaced(Platform::String ^callId)
{
}


void RingClientUWP::MainPage::OnnameRegistred(bool status)
{
    showLoadingOverlay(false, false);
}


void RingClientUWP::MainPage::OnvolatileDetailsChanged(const std::string &accountId, const std::map<std::string, std::string>& details)
{
    showLoadingOverlay(false, false);
}

void RingClientUWP::MainPage::OnFullScreenToggled(bool state)
{
    static bool openState;
    if (state == true) {
        openState = _innerSplitView_->IsPaneOpen;
        _innerSplitView_->IsPaneOpen = false;
        _innerSplitView_->CompactPaneLength = 0;
    }
    else {
        _innerSplitView_->IsPaneOpen = openState;
        _innerSplitView_->CompactPaneLength = 60;
    }
}