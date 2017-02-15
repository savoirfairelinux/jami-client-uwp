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

#include "VideoPage.xaml.h"

#include <MemoryBuffer.h>   // IMemoryBufferByteAccess

using namespace RingClientUWP::Views;
using namespace ViewModel;
using namespace Video;

using namespace Concurrency;
using namespace Platform;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Media::Capture;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Windows::UI;

using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Media::Capture;
using namespace Windows::Devices::Sensors;

using namespace Windows::UI::Input;

VideoPage::VideoPage()
{
    InitializeComponent();

    Page::NavigationCacheMode = Navigation::NavigationCacheMode::Required;

    VideoManager::instance->rendererManager()->writeVideoFrame +=
        ref new WriteVideoFrame([this](String^ id, uint8_t* buf, int width, int height)
    {
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
        ref new DispatchedHandler([=]() {
            try {
                auto renderer_w = VideoManager::instance->rendererManager()->renderer(id);
                if (!renderer_w) {
                    return;
                }
                else {
                    renderer_w->isRendering = true;
                    create_task(WriteFrameAsSoftwareBitmapAsync(id, buf, width, height))
                    .then([=](task<void> previousTask) {
                        try {
                            previousTask.get();
                        }
                        catch (Platform::Exception^ e) {
                            MSG_( "Caught exception from WriteFrameAsSoftwareBitmapAsync task.\n" );
                        }
                    });
                }
            }
            catch(Platform::COMException^ e) {
                EXC_(e);
            }
        }));
    });

    VideoManager::instance->captureManager()->startPreviewing +=
        ref new StartPreviewing([this]()
    {
        PreviewImage->Visibility = Windows::UI::Xaml::Visibility::Visible;
        PreviewImage->FlowDirection = VideoManager::instance->captureManager()->mirroringPreview ?
                                      Windows::UI::Xaml::FlowDirection::RightToLeft :
                                      Windows::UI::Xaml::FlowDirection::LeftToRight;
        updatePreviewFrameDimensions();
    });

    VideoManager::instance->captureManager()->stopPreviewing +=
        ref new StopPreviewing([this]()
    {
        PreviewImage->Source = nullptr;
        PreviewImage->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    });

    VideoManager::instance->captureManager()->getSink +=
        ref new GetSink([this]()
    {
        return PreviewImage;
    });

    VideoManager::instance->rendererManager()->clearRenderTarget +=
        ref new ClearRenderTarget([this]()
    {
        IncomingVideoImage->Source = nullptr;
    });

    RingD::instance->windowResized +=
        ref new WindowResized([&]()
    {
        updatePreviewFrameDimensions();
    });

    RingD::instance->incomingAccountMessage +=
        ref new IncomingAccountMessage([&](String^ accountId, String^ from, String^ payload)
    {
        scrollDown();
    });

    RingD::instance->stateChange +=
        ref new StateChange([&](String^ callId, CallStatus state, int code)
    {
        switch (state) {
        case CallStatus::IN_PROGRESS:
        {
            for (auto it : SmartPanelItemsViewModel::instance->itemsList)
                if (it->_callStatus != CallStatus::IN_PROGRESS && it->_callId != callId)
                    RingD::instance->pauseCall(Utils::toString(it->_callId));

            _callPaused_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            IncomingVideoImage->Visibility = Windows::UI::Xaml::Visibility::Visible;
//            PreviewImage->Visibility = Windows::UI::Xaml::Visibility::Visible;
            break;
        }
        case CallStatus::ENDED:
        {
            Video::VideoManager::instance->rendererManager()->raiseClearRenderTarget();

            if (RingD::instance->isFullScreen)
                RingD::instance->setWindowedMode();

            /* "close" the chat panel */
            _rowChatBx_->Height = 0;

            break;
        }
        case CallStatus::PEER_PAUSED:
        case CallStatus::PAUSED:
            _callPaused_->Visibility = Windows::UI::Xaml::Visibility::Visible;
            IncomingVideoImage->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
//            PreviewImage->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            break;
        }
    });

    RingD::instance->incomingMessage += ref new RingClientUWP::IncomingMessage(this, &RingClientUWP::Views::VideoPage::OnincomingMessage);
    RingD::instance->incomingVideoMuted += ref new RingClientUWP::IncomingVideoMuted(this, &RingClientUWP::Views::VideoPage::OnincomingVideoMuted);
    VideoManager::instance->captureManager()->startPreviewing += ref new RingClientUWP::StartPreviewing(this, &RingClientUWP::Views::VideoPage::OnstartPreviewing);
    VideoManager::instance->captureManager()->stopPreviewing += ref new RingClientUWP::StopPreviewing(this, &RingClientUWP::Views::VideoPage::OnstopPreviewing);
    RingD::instance->audioMuted += ref new RingClientUWP::AudioMuted(this, &RingClientUWP::Views::VideoPage::OnaudioMuted);
    RingD::instance->videoMuted += ref new RingClientUWP::VideoMuted(this, &RingClientUWP::Views::VideoPage::OnvideoMuted);

    InitManipulationTransforms();

    PreviewImage->ManipulationDelta += ref new ManipulationDeltaEventHandler(this, &VideoPage::PreviewImage_ManipulationDelta);
    PreviewImage->ManipulationCompleted += ref new ManipulationCompletedEventHandler(this, &VideoPage::PreviewImage_ManipulationCompleted);

    PreviewImageResizer->ManipulationDelta += ref new ManipulationDeltaEventHandler(this, &VideoPage::PreviewImageResizer_ManipulationDelta);
    PreviewImageResizer->ManipulationCompleted += ref new ManipulationCompletedEventHandler(this, &VideoPage::PreviewImageResizer_ManipulationCompleted);

    PreviewImage->ManipulationMode =
        ManipulationModes::TranslateX |
        ManipulationModes::TranslateY;

    PreviewImageResizer->ManipulationMode = ManipulationModes::TranslateY;
}

void
RingClientUWP::Views::VideoPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    updatePageContent();
    _rowChatBx_->Height = 0;
}

void RingClientUWP::Views::VideoPage::updatePageContent()
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = (item) ? item->_contact : nullptr;

    if (!contact)
        return;

    _callee_->Text = contact->_name;

    _messagesList_->ItemsSource = contact->_conversation->_messages;

    scrollDown();
}

void
VideoPage::updatePreviewFrameDimensions()
{
    auto resolution = VideoManager::instance->captureManager()->activeDevice->currentResolution();
    double aspectRatio = static_cast<double>(resolution->width()) / static_cast<double>(resolution->height());
    PreviewImage->Height = userPreviewHeightModifier + ( _videoContent_->ActualHeight / 4 );
    PreviewImage->Width = PreviewImage->Height * aspectRatio;

    TranslateTransform^ translate = ref new TranslateTransform();
    translate->Y = - userPreviewHeightModifier;
    translate->X = translate->Y * aspectRatio;

    PreviewImage->RenderTransform = translate;
}

void RingClientUWP::Views::VideoPage::scrollDown()
{
    _scrollView_->UpdateLayout();
    _scrollView_->ScrollToVerticalOffset(_scrollView_->ScrollableHeight);
}

void RingClientUWP::Views::VideoPage::screenVideo(bool state)
{
    if (state) {
        Video::VideoManager::instance->rendererManager()->raiseClearRenderTarget();
        _callPaused_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        IncomingVideoImage->Visibility = Windows::UI::Xaml::Visibility::Visible;
        PreviewImage->Visibility = Windows::UI::Xaml::Visibility::Visible;
    } else {
        _callPaused_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        IncomingVideoImage->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        PreviewImage->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }


}

void
RingClientUWP::Views::VideoPage::_sendBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    sendMessage();
}

void
RingClientUWP::Views::VideoPage::_messageTextBox__KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e)
{
    if (e->Key == Windows::System::VirtualKey::Enter) {
        sendMessage();
    }
}

void
RingClientUWP::Views::VideoPage::sendMessage()
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = item->_contact;

    auto txt = _messageTextBox_->Text;

    /* empty the textbox */
    _messageTextBox_->Text = "";

    if (!contact || txt->IsEmpty())
        return;

    RingD::instance->sendSIPTextMessage(txt);
    scrollDown();
}

void RingClientUWP::Views::VideoPage::Button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
}


void RingClientUWP::Views::VideoPage::_btnCancel__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{

}

void RingClientUWP::Views::VideoPage::_btnHangUp__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;

    if (item) {
        RingD::instance->hangUpCall2(item->_callId);
        pressHangUpCall();
    }
    else
        WNG_("item not found, cannot hang up");
}


void RingClientUWP::Views::VideoPage::_btnPause__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    if (item->_callStatus == CallStatus::IN_PROGRESS)
        RingD::instance->pauseCall(Utils::toString(item->_callId));
    else if (item->_callStatus == CallStatus::PAUSED)
        RingD::instance->unPauseCall(Utils::toString(item->_callId));

    pauseCall();
}


void RingClientUWP::Views::VideoPage::_btnChat__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    if (_rowChatBx_->Height == 0) {
        _rowChatBx_->Height = 200;
        SmartPanelItemsViewModel::instance->_selectedItem->_contact->_unreadMessages = 0;
    }
    else {
        _rowChatBx_->Height = 0;
    }
}


void RingClientUWP::Views::VideoPage::_btnAddFriend__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    addContactCall();
}


void RingClientUWP::Views::VideoPage::_btnSwitch__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    transferCall();
}


void RingClientUWP::Views::VideoPage::_btnMicrophone__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    switchMicrophoneStateCall();
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;

    auto state = !item->_audioMuted;
    item->_audioMuted = state;

    // refacto : compare how video and audios are muted, then decide which solution is best.
    RingD::instance->muteAudio(Utils::toString(item->_callId), state); // nb : muteAudio == setMuteAudio
}


void RingClientUWP::Views::VideoPage::_btnMemo__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    reccordVideoCall();
}


void RingClientUWP::Views::VideoPage::_btnHQ__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    qualityVideoLevelCall();
}


void RingClientUWP::Views::VideoPage::_btnVideo__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    switchVideoStateCall();
}


void RingClientUWP::Views::VideoPage::_videoControl__PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    if (barFading)
        myStoryboard->Begin();
    barFading_ = true;
}


void RingClientUWP::Views::VideoPage::btnAny_entered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    barFading_ = false;
    myStoryboard->Stop();
}


void RingClientUWP::Views::VideoPage::btnAny_exited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    barFading_ = true;
}

task<void>
VideoPage::WriteFrameAsSoftwareBitmapAsync(String^ id, uint8_t* buf, int width, int height)
{
    auto vframe = ref new VideoFrame(BitmapPixelFormat::Bgra8, width, height);
    auto frame = vframe->SoftwareBitmap;

    const int BYTES_PER_PIXEL = 4;

    BitmapBuffer^ buffer = frame->LockBuffer(BitmapBufferAccessMode::ReadWrite);
    IMemoryBufferReference^ reference = buffer->CreateReference();

    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> byteAccess;
    if (SUCCEEDED(reinterpret_cast<IUnknown*>(reference)->QueryInterface(IID_PPV_ARGS(&byteAccess))))
    {
        byte* data;
        unsigned capacity;
        byteAccess->GetBuffer(&data, &capacity);
        auto desc = buffer->GetPlaneDescription(0);
        std::memcpy(data, buf, static_cast<size_t>(capacity));
    }
    delete reference;
    delete buffer;

    VideoManager::instance->rendererManager()->renderer(id)->isRendering = false;

    auto sbSource = ref new Media::Imaging::SoftwareBitmapSource();
    return create_task(sbSource->SetBitmapAsync(frame))
           .then([this, sbSource]()
    {
        try {
            IncomingVideoImage->Source = sbSource;
        }
        catch (Exception^ e) {
            WriteException(e);
        }
    });
}


void RingClientUWP::Views::VideoPage::OnincomingMessage(Platform::String ^callId, Platform::String ^payload)
{
    if (_rowChatBx_->Height == 0)
        _rowChatBx_->Height = 200;

    scrollDown();
}


void RingClientUWP::Views::VideoPage::_btnVideo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;

    item->muteVideo(!item->_videoMuted);
}


void RingClientUWP::Views::VideoPage::OnincomingVideoMuted(Platform::String ^callId, bool state)
{
    /*_callPaused_->Visibility = (state)
                                   ? Windows::UI::Xaml::Visibility::Visible
                                   : Windows::UI::Xaml::Visibility::Collapsed;*/

    IncomingVideoImage->Visibility = (state)
                                     ? Windows::UI::Xaml::Visibility::Collapsed
                                     : Windows::UI::Xaml::Visibility::Visible;
}


void RingClientUWP::Views::VideoPage::OnstartPreviewing()
{
    PreviewImage->Visibility = Windows::UI::Xaml::Visibility::Visible;
}


void RingClientUWP::Views::VideoPage::OnstopPreviewing()
{
    PreviewImage->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
}


void RingClientUWP::Views::VideoPage::_btnMicrophone__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    switchMicrophoneStateCall();
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;

    auto state = !item->_audioMuted;
    item->_audioMuted = state;

    // refacto : compare how video and audios are muted, then decide which solution is best.
    RingD::instance->muteAudio(Utils::toString(item->_callId), state); // nb : muteAudio == setMuteAudio
}


void RingClientUWP::Views::VideoPage::OnaudioMuted(const std::string &callId, bool state)
{
    _txbkMicrophoneMuted_->Visibility = (state) ? Windows::UI::Xaml::Visibility::Visible
                                        : Windows::UI::Xaml::Visibility::Collapsed;
}


void RingClientUWP::Views::VideoPage::OnvideoMuted(const std::string &callId, bool state)
{
    _txbkVideoMuted_->Visibility = (state) ? Windows::UI::Xaml::Visibility::Visible
                                   : Windows::UI::Xaml::Visibility::Collapsed;
}


void RingClientUWP::Views::VideoPage::IncomingVideoImage_DoubleTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::DoubleTappedRoutedEventArgs^ e)
{
    RingD::instance->toggleFullScreen();
}


void RingClientUWP::Views::VideoPage::InitManipulationTransforms()
{
    PreviewImage_transforms = ref new TransformGroup();
    PreviewImage_previousTransform = ref new MatrixTransform();
    PreviewImage_previousTransform->Matrix = Matrix::Identity;
    PreviewImage_deltaTransform = ref new CompositeTransform();

    PreviewImage_transforms->Children->Append(PreviewImage_previousTransform);
    PreviewImage_transforms->Children->Append(PreviewImage_deltaTransform);

    PreviewImageRect->RenderTransform = PreviewImage_transforms;
}

void
VideoPage::DebugMetrics()
{
    auto previewRectWidth = PreviewImageRect->ActualWidth;
    auto previewRectHeight = PreviewImageRect->ActualHeight;
    auto previewWidth = PreviewImage->ActualWidth;
    auto previewHeight = PreviewImage->ActualHeight;

    MSG_("PR: [" + previewRectWidth.ToString() + ", " + previewRectHeight.ToString() + "]");
    MSG_("PI: [" + previewWidth.ToString() + ", " + previewHeight.ToString() + "]");
}

void RingClientUWP::Views::VideoPage::PreviewImage_ManipulationDelta(Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ e)
{
    PreviewImage_previousTransform->Matrix = PreviewImage_transforms->Value;

    //DebugMetrics();

    PreviewImage_deltaTransform->TranslateX = e->Delta.Translation.X;
    PreviewImage_deltaTransform->TranslateY = e->Delta.Translation.Y;

    // clip scale to frame
}

void RingClientUWP::Views::VideoPage::PreviewImage_ManipulationCompleted(Platform::Object^ sender, ManipulationCompletedRoutedEventArgs^ e)
{
    // snap oversized
}

void RingClientUWP::Views::VideoPage::PreviewImageResizer_ManipulationDelta(Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ e)
{
    //PreviewImage_previousTransform->Matrix = PreviewImage_transforms->Value;

    DebugMetrics();

    userPreviewHeightModifier -= e->Delta.Translation.Y;

    updatePreviewFrameDimensions();

    MSG_("userScaleValue: " + userPreviewHeightModifier.ToString());

    //PreviewImage_deltaTransform->ScaleX = previewImageScale;
    //PreviewImage_deltaTransform->ScaleY = previewImageScale;

    // clip translation to frame
}

void RingClientUWP::Views::VideoPage::PreviewImageResizer_ManipulationCompleted(Platform::Object^ sender, ManipulationCompletedRoutedEventArgs^ e)
{
    // magnetic snap
}