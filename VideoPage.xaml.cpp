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
using namespace Windows::UI::Xaml::Media::Animation;
using namespace Windows::UI::Xaml::Shapes;
using namespace Windows::Devices::Sensors;

using namespace Windows::UI::Input;

VideoPage::VideoPage()
{
    InitializeComponent();

    barFading = true;

    Page::NavigationCacheMode = Navigation::NavigationCacheMode::Required;

    VideoManager::instance->rendererManager()->writeVideoFrame +=
        ref new WriteVideoFrame([this](String^ id, uint8_t* buf, int width, int height)
    {
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
        ref new DispatchedHandler([=]() {
            try {
                currentRendererWrapper = VideoManager::instance->rendererManager()->renderer(id);
                if (!currentRendererWrapper) {
                    return;
                }
                else {
                    currentRendererWrapper->isRendering = true;
                    create_task(WriteFrameAsSoftwareBitmapAsync(id, buf, width, height))
                    .then([=](task<void> previousTask) {
                        try {
                            previousTask.get();
                        }
                        catch (Platform::Exception^ e) {
                            EXC_(e);
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
        _PreviewImage_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _PreviewImageResizer_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _PreviewImage_->FlowDirection = VideoManager::instance->captureManager()->mirroringPreview ?
                                      Windows::UI::Xaml::FlowDirection::RightToLeft :
                                      Windows::UI::Xaml::FlowDirection::LeftToRight;

        double aspectRatio = VideoManager::instance->captureManager()->aspectRatio();

        _PreviewImage_->Height = ( _videoContent_->ActualHeight / 4 );
        _PreviewImage_->Width = _PreviewImage_->Height * aspectRatio;
        _PreviewImageRect_->Width = _PreviewImage_->Width;
        _PreviewImageRect_->Height = _PreviewImage_->Height;
    });

    VideoManager::instance->captureManager()->stopPreviewing +=
        ref new StopPreviewing([this]()
    {
        _PreviewImage_->Source = nullptr;
        _PreviewImage_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _PreviewImageResizer_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    });

    VideoManager::instance->captureManager()->getSink +=
        ref new GetSink([this]()
    {
        return _PreviewImage_;
    });

    VideoManager::instance->rendererManager()->clearRenderTarget +=
        ref new ClearRenderTarget([this]()
    {
        _IncomingVideoImage_->Source = nullptr;
    });

    RingD::instance->windowResized +=
        ref new WindowResized([=](float width, float height)
    {
    });

    RingD::instance->incomingAccountMessage +=
        ref new IncomingAccountMessage([&](String^ accountId, String^ from, String^ payload)
    {
        scrollDown();
    });

    RingD::instance->vCardUpdated += ref new VCardUpdated([&](Contact^ contact)
    {
        Utils::runOnUIThread([this, contact]() {
            SmartPanelItemsViewModel::instance->update({ "_bestName2", "_avatarImage" });
            updatePageContent();
        });
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
            _IncomingVideoImage_->Visibility = Windows::UI::Xaml::Visibility::Visible;

            callTimerUpdater->Start();
            updateCallTimer(nullptr, nullptr);

            RingD::instance->startSmartInfo(500);
            videoPage_InputHandlerToken = Window::Current->CoreWindow->KeyDown +=
                ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &VideoPage::_corewindow__KeyDown);

            break;
        }
        case CallStatus::ENDED:
        {
            Video::VideoManager::instance->rendererManager()->raiseClearRenderTarget();

            if (RingD::instance->isFullScreen)
                RingD::instance->setWindowedMode();

            /* "close" the chat panel */
            closeChatPanel();

            callTimerUpdater->Stop();

            //RingD::instance->stopSmartInfo();
            Window::Current->CoreWindow->KeyDown -= videoPage_InputHandlerToken;
            _smartInfoBorder_->Visibility = VIS::Collapsed;

            break;
        }
        case CallStatus::PEER_PAUSED:
        case CallStatus::PAUSED:
            _callPaused_->Visibility = Windows::UI::Xaml::Visibility::Visible;
            _IncomingVideoImage_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
            break;
        }
    });

    RingD::instance->messageDataLoaded += ref new MessageDataLoaded([&]() { scrollDown(); });

    RingD::instance->updateSmartInfo += ref new RingClientUWP::UpdateSmartInfo(this, &RingClientUWP::Views::VideoPage::OnsmartInfoUpdated);

    RingD::instance->incomingMessage += ref new RingClientUWP::IncomingMessage(this, &RingClientUWP::Views::VideoPage::OnincomingMessage);
    RingD::instance->incomingVideoMuted += ref new RingClientUWP::IncomingVideoMuted(this, &RingClientUWP::Views::VideoPage::OnincomingVideoMuted);
    VideoManager::instance->captureManager()->startPreviewing += ref new RingClientUWP::StartPreviewing(this, &RingClientUWP::Views::VideoPage::OnstartPreviewing);
    VideoManager::instance->captureManager()->stopPreviewing += ref new RingClientUWP::StopPreviewing(this, &RingClientUWP::Views::VideoPage::OnstopPreviewing);
    RingD::instance->audioMuted += ref new RingClientUWP::AudioMuted(this, &RingClientUWP::Views::VideoPage::OnaudioMuted);
    RingD::instance->videoMuted += ref new RingClientUWP::VideoMuted(this, &RingClientUWP::Views::VideoPage::OnvideoMuted);

    InitManipulationTransforms();

    _PreviewImage_->ManipulationDelta += ref new ManipulationDeltaEventHandler(this, &VideoPage::PreviewImage_ManipulationDelta);
    _PreviewImage_->ManipulationCompleted += ref new ManipulationCompletedEventHandler(this, &VideoPage::PreviewImage_ManipulationCompleted);
    _PreviewImage_->ManipulationMode =
        ManipulationModes::TranslateX |
        ManipulationModes::TranslateY;

    _PreviewImageResizer_->ManipulationDelta += ref new ManipulationDeltaEventHandler(this, &VideoPage::PreviewImageResizer_ManipulationDelta);
    _PreviewImageResizer_->ManipulationCompleted += ref new ManipulationCompletedEventHandler(this, &VideoPage::PreviewImageResizer_ManipulationCompleted);
    _PreviewImageResizer_->ManipulationMode = ManipulationModes::TranslateY;

    _chatPanelResizeBarGrid_->ManipulationDelta += ref new ManipulationDeltaEventHandler(this, &VideoPage::_chatPanelResizeBarGrid__ManipulationDelta);
    _chatPanelResizeBarGrid_->ManipulationCompleted += ref new ManipulationCompletedEventHandler(this, &VideoPage::_chatPanelResizeBarGrid__ManipulationCompleted);
    _chatPanelResizeBarGrid_->ManipulationMode =
        ManipulationModes::TranslateX |
        ManipulationModes::TranslateY;

    TimeSpan timeSpan;
    timeSpan.Duration = static_cast<long long>(1e7);
    callTimerUpdater = ref new DispatcherTimer;
    callTimerUpdater->Interval = timeSpan;
    callTimerUpdater->Tick += ref new Windows::Foundation::EventHandler<Object^>(this, &VideoPage::updateCallTimer);

    showSmartInfo = false;
}

void
VideoPage::OnsmartInfoUpdated(const std::map<std::string, std::string>& info)
{
    auto smartInfo = Utils::convertMap(info);

    if (auto selectedItem = SmartPanelItemsViewModel::instance->_selectedItem)
        _si_CallId_->Text = "CallID: " + selectedItem->_callId;

    if (smartInfo->HasKey("local FPS"))
        _si_fps1_->Text = smartInfo->Lookup("local FPS");
    if (smartInfo->HasKey("local video codec"))
        _si_vc1_->Text = smartInfo->Lookup("local video codec");
    if (smartInfo->HasKey("local audio codec"))
        _si_ac1_->Text = smartInfo->Lookup("local audio codec");

    auto localResolution = VideoManager::instance->captureManager()->activeDevice->currentResolution();
    _si_res1_->Text = localResolution->getFriendlyName();

    if (smartInfo->HasKey("remote FPS"))
        _si_fps2_->Text = smartInfo->Lookup("remote FPS") ;
    if (smartInfo->HasKey("remote video codec"))
        _si_vc2_->Text = smartInfo->Lookup("remote video codec");
    if (smartInfo->HasKey("remote audio codec"))
        _si_ac2_->Text = smartInfo->Lookup("remote audio codec");

    if (currentRendererWrapper) {
        auto remoteResolution = currentRendererWrapper->renderer->width.ToString() +
                                "x" +
                                currentRendererWrapper->renderer->height.ToString();
        _si_res2_->Text = remoteResolution;
    }
}

void
VideoPage::updateCallTimer(Object^ sender, Object^ e)
{
    if(auto item = SmartPanelItemsViewModel::instance->_selectedItem)
        _callTime_->Text = item->_callTime;
}

void
VideoPage::OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e)
{
    updatePageContent();
    closeChatPanel();
}

void RingClientUWP::Views::VideoPage::updatePageContent()
{
    auto item = SmartPanelItemsViewModel::instance->_selectedItem;
    auto contact = (item) ? item->_contact : nullptr;

    if (!contact)
        return;

    _contactName_->Text = contact->_bestName;

    //_contactBarAvatar_->ImageSource = RingClientUWP::ResourceMananger::instance->imageFromRelativePath(contact->_avatarImage);
    if (contact->_avatarImage != " ") {
        auto avatarImageUri = ref new Windows::Foundation::Uri(contact->_avatarImage);
        _contactBarAvatar_->ImageSource = ref new BitmapImage(avatarImageUri);
        _defaultContactBarAvatarGrid_->Visibility = VIS::Collapsed;
        _contactBarAvatarGrid_->Visibility = VIS::Visible;
    }
    else {
        _defaultContactBarAvatarGrid_->Visibility = VIS::Visible;
        _contactBarAvatarGrid_->Visibility = VIS::Collapsed;
        _defaultAvatar_->Fill = contact->_avatarColorBrush;
        _defaultAvatarInitial_->Text = Utils::getUpperInitial(contact->_bestName2);
    }

    _messagesList_->ItemsSource = contact->_conversation->_messages;

    scrollDown();
}

void
VideoPage::updatePreviewFrameDimensions()
{
    double aspectRatio = VideoManager::instance->captureManager()->aspectRatio();

    TransformGroup^ transforms = ref new TransformGroup();

    double scaleValue = 1 + userPreviewHeightModifier / _PreviewImage_->Height;

    scaleValue = std::max(std::min(1.75, scaleValue), 0.5);

    userPreviewHeightModifier = _PreviewImage_->Height * (scaleValue - 1);

    ScaleTransform^ scale = ref new ScaleTransform();
    scale->ScaleX = scaleValue;
    scale->ScaleY = scaleValue;

    TranslateTransform^ translate = ref new TranslateTransform();
    switch (quadrant)
    {
    case Quadrant::SE:
        translate->Y = -userPreviewHeightModifier;
        translate->X = translate->Y * aspectRatio;
        break;
    case Quadrant::SW:
        translate->Y = -userPreviewHeightModifier;
        translate->X = 0;
        break;
    case Quadrant::NW:
        translate->Y = 0;
        translate->X = 0;
        break;
    case Quadrant::NE:
        translate->Y = 0;
        translate->X = -userPreviewHeightModifier * aspectRatio;
        break;
    default:
        break;
    }

    transforms->Children->Append(scale);
    transforms->Children->Append(translate);

    _PreviewImage_->RenderTransform = transforms;

    _PreviewImageResizer_->RenderTransform = translate;

    arrangeResizer();
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
        _IncomingVideoImage_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _PreviewImage_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _PreviewImageResizer_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    } else {
        _callPaused_->Visibility = Windows::UI::Xaml::Visibility::Visible;
        _IncomingVideoImage_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _PreviewImage_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _PreviewImageResizer_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    }
}

void
RingClientUWP::Views::VideoPage::_sendBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    sendMessage();
}

void
RingClientUWP::Views::VideoPage::sendMessage()
{
    if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
        auto contact = item->_contact;

        auto txt = _messageTextBox_->Text;

        /* empty the textbox */
        _messageTextBox_->Text = "";

        if (!contact || txt->IsEmpty())
            return;

        RingD::instance->sendSIPTextMessage(txt);
        scrollDown();
    }
}

void RingClientUWP::Views::VideoPage::Button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
}


void RingClientUWP::Views::VideoPage::_btnCancel__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
}

void RingClientUWP::Views::VideoPage::_btnHangUp__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    if (auto item = SmartPanelItemsViewModel::instance->_selectedItem) {
        _PreviewImage_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
        _PreviewImageResizer_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
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
    if (!isChatPanelOpen) {
        openChatPanel();
    }
    else {
        closeChatPanel();
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
        fadeVideoControlsStoryboard->Begin();
}


void RingClientUWP::Views::VideoPage::btnAny_entered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    barFading = false;
    fadeVideoControlsStoryboard->Stop();
}


void RingClientUWP::Views::VideoPage::btnAny_exited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    barFading = true;
}

void
VideoPage::SetBuffer(uint8_t* buf, int width, int height)
{
    videoFrame = ref new VideoFrame(BitmapPixelFormat::Bgra8, width, height);
    softwareBitmap = videoFrame->SoftwareBitmap;

    BitmapBuffer^ bitmapBuffer = softwareBitmap->LockBuffer(BitmapBufferAccessMode::Write);
    IMemoryBufferReference^ memoryBufferReference = bitmapBuffer->CreateReference();

    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> byteAccess;
    if (SUCCEEDED(reinterpret_cast<IUnknown*>(memoryBufferReference)->QueryInterface(IID_PPV_ARGS(&byteAccess))))
    {
        byte* data;
        unsigned capacity;
        byteAccess->GetBuffer(&data, &capacity);
        std::memcpy(data, buf, static_cast<size_t>(capacity));
    }
    delete memoryBufferReference;
    delete bitmapBuffer;
}


task<void>
VideoPage::WriteFrameAsSoftwareBitmapAsync(String^ id, uint8_t* buf, int width, int height)
{
    SetBuffer(buf, width, height);

    VideoManager::instance->rendererManager()->renderer(id)->isRendering = false;

    auto sbSource = ref new Media::Imaging::SoftwareBitmapSource();
    return create_task(sbSource->SetBitmapAsync(softwareBitmap))
           .then([this, sbSource]()
    {
        try {
            _IncomingVideoImage_->Source = sbSource;
        }
        catch (Exception^ e) {
            EXC_(e);
        }
    });
}


void RingClientUWP::Views::VideoPage::OnincomingMessage(Platform::String ^callId, Platform::String ^payload)
{
    openChatPanel();
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

    _IncomingVideoImage_->Visibility = (state)
                                     ? Windows::UI::Xaml::Visibility::Collapsed
                                     : Windows::UI::Xaml::Visibility::Visible;
}


void RingClientUWP::Views::VideoPage::OnstartPreviewing()
{
    _PreviewImage_->Visibility = Windows::UI::Xaml::Visibility::Visible;
    _PreviewImageResizer_->Visibility = Windows::UI::Xaml::Visibility::Visible;
}


void RingClientUWP::Views::VideoPage::OnstopPreviewing()
{
    _PreviewImage_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
    _PreviewImageResizer_->Visibility = Windows::UI::Xaml::Visibility::Collapsed;
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
    anchorPreview();
}

void RingClientUWP::Views::VideoPage::InitManipulationTransforms()
{
    PreviewImage_transforms = ref new TransformGroup();
    PreviewImage_previousTransform = ref new MatrixTransform();
    PreviewImage_previousTransform->Matrix = Matrix::Identity;
    PreviewImage_deltaTransform = ref new CompositeTransform();

    PreviewImage_transforms->Children->Append(PreviewImage_previousTransform);
    PreviewImage_transforms->Children->Append(PreviewImage_deltaTransform);

    _PreviewImageRect_->RenderTransform = PreviewImage_transforms;
}

void RingClientUWP::Views::VideoPage::PreviewImage_ManipulationDelta(Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ e)
{
    if (!isMovingPreview)
        isMovingPreview = true;

    _PreviewImageRect_->RenderTransform = PreviewImage_transforms;

    PreviewImage_previousTransform->Matrix = PreviewImage_transforms->Value;

    PreviewImage_deltaTransform->TranslateX = e->Delta.Translation.X;
    PreviewImage_deltaTransform->TranslateY = e->Delta.Translation.Y;

    computeQuadrant();
}

void
RingClientUWP::Views::VideoPage::computeQuadrant()
{
    // Compute center coordinate of _videoContent_
    Point centerOfVideoFrame = Point(   static_cast<float>(_videoContent_->ActualWidth - _colChatBx_->ActualWidth) / 2,
                                        static_cast<float>(_videoContent_->ActualHeight - _rowChatBx_->ActualHeight) / 2  );

    // Compute the center coordinate of _PreviewImage_ relative to _videoContent_
    Point centerOfPreview = Point( static_cast<float>(_PreviewImage_->ActualWidth) / 2,
                                   static_cast<float>(_PreviewImage_->ActualHeight) / 2 );
    UIElement^ container = dynamic_cast<UIElement^>(VisualTreeHelper::GetParent(_videoContent_));
    GeneralTransform^ transform = _PreviewImage_->TransformToVisual(container);
    Point relativeCenterOfPreview = transform->TransformPoint(centerOfPreview);

    // Compute the difference between the center of _videoContent_
    // and the relative scaled center of _PreviewImageRect_
    Point diff = Point( centerOfVideoFrame.X - relativeCenterOfPreview.X,
                        centerOfVideoFrame.Y - relativeCenterOfPreview.Y    );

    lastQuadrant = quadrant;
    if (diff.X > 0)
        quadrant = diff.Y > 0 ? Quadrant::NW : Quadrant::SW;
    else
        quadrant = diff.Y > 0 ? Quadrant::NE : Quadrant::SE;

    if (lastQuadrant != quadrant) {
        arrangeResizer();
    }
}

void
RingClientUWP::Views::VideoPage::arrangeResizer()
{
    double scaleValue = (userPreviewHeightModifier + _PreviewImage_->Height) / _PreviewImage_->Height;
    float scaledWidth = static_cast<float>(scaleValue * _PreviewImage_->ActualWidth);
    float scaledHeight = static_cast<float>(scaleValue * _PreviewImage_->ActualHeight);

    float rSize = 20; // the size of the square UIElement used to resize the preview
    float xOffset, yOffset;
    PointCollection^ resizeTrianglePoints = ref new PointCollection();
    switch (quadrant)
    {
    case Quadrant::SE:
        xOffset = 0;
        yOffset = 0;
        resizeTrianglePoints->Append(Point(xOffset,         yOffset));
        resizeTrianglePoints->Append(Point(xOffset + rSize, yOffset));
        resizeTrianglePoints->Append(Point(xOffset,         yOffset + rSize));
        break;
    case Quadrant::SW:
        xOffset = scaledWidth - rSize;
        yOffset = 0;
        resizeTrianglePoints->Append(Point(xOffset,         yOffset));
        resizeTrianglePoints->Append(Point(xOffset + rSize, yOffset));
        resizeTrianglePoints->Append(Point(xOffset + rSize, yOffset + rSize));
        break;
    case Quadrant::NW:
        xOffset = scaledWidth - rSize;
        yOffset = scaledHeight - rSize;
        resizeTrianglePoints->Append(Point(xOffset + rSize, yOffset));
        resizeTrianglePoints->Append(Point(xOffset + rSize, yOffset + rSize));
        resizeTrianglePoints->Append(Point(xOffset,         yOffset + rSize));
        break;
    case Quadrant::NE:
        xOffset = 0;
        yOffset = scaledHeight - rSize;
        resizeTrianglePoints->Append(Point(xOffset,         yOffset + rSize));
        resizeTrianglePoints->Append(Point(xOffset + rSize, yOffset + rSize));
        resizeTrianglePoints->Append(Point(xOffset,         yOffset));
        break;
    default:
        break;
    }
    _PreviewImageResizer_->Points = resizeTrianglePoints;
}

void RingClientUWP::Views::VideoPage::PreviewImage_ManipulationCompleted(Platform::Object^ sender, ManipulationCompletedRoutedEventArgs^ e)
{
    isMovingPreview = false;
    anchorPreview();
    updatePreviewFrameDimensions();
}

void
VideoPage::PreviewImageResizer_Pressed(Object^ sender, PointerRoutedEventArgs^ e)
{
    isResizingPreview = true;
    lastUserPreviewHeightModifier = userPreviewHeightModifier;
}

void RingClientUWP::Views::VideoPage::anchorPreview()
{
    PreviewImage_previousTransform->Matrix = Matrix::Identity;
    _PreviewImageRect_->RenderTransform =  nullptr;

    switch (quadrant)
    {
    case Quadrant::SE:
        _PreviewImageRect_->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Right;
        _PreviewImageRect_->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Bottom;
        break;
    case Quadrant::SW:
        _PreviewImageRect_->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Left;
        _PreviewImageRect_->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Bottom;
        break;
    case Quadrant::NW:
        _PreviewImageRect_->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Left;
        _PreviewImageRect_->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Top;
        break;
    case Quadrant::NE:
        _PreviewImageRect_->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Right;
        _PreviewImageRect_->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Top;
        break;
    default:
        break;
    };
}

void
VideoPage::PreviewImageResizer_ManipulationDelta(Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ e)
{
    if (!isResizingPreview)
        isResizingPreview = true;

    if (quadrant == Quadrant::NW || quadrant == Quadrant::NE) {
        userPreviewHeightModifier = lastUserPreviewHeightModifier + e->Cumulative.Translation.Y;
    }
    else {
        userPreviewHeightModifier = lastUserPreviewHeightModifier - (e->Cumulative.Translation.Y);
    }

    updatePreviewFrameDimensions();
}

void
VideoPage::PreviewImageResizer_ManipulationCompleted(Platform::Object^ sender, ManipulationCompletedRoutedEventArgs^ e)
{
    isResizingPreview = false;
    if (!isHoveringOnResizer) {
        CoreApplication::MainView->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::Arrow, 0);
    }
}

void
VideoPage::PreviewImage_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    // For some reason, PreviewImage_ManipulationCompleted doesn't always fired when the mouse is released
    anchorPreview();
    updatePreviewFrameDimensions();
}

void
VideoPage::PreviewImageResizer_PointerEntered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    isHoveringOnResizer = true;
    if (!isMovingPreview) {
        switch (quadrant)
        {
        case Quadrant::SE:
        case Quadrant::NW:
            CoreApplication::MainView->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::SizeNorthwestSoutheast, 0);
            break;
        case Quadrant::SW:
        case Quadrant::NE:
            CoreApplication::MainView->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::SizeNortheastSouthwest, 0);
            break;
        default:
            break;
        }
    }
}

void
VideoPage::PreviewImageResizer_PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    isHoveringOnResizer = false;
    if (!isResizingPreview) {
        CoreApplication::MainView->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::Arrow, 0);
    }
}

void
VideoPage::PreviewImageResizer_PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    if (!isHoveringOnResizer) {
        CoreApplication::MainView->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::Arrow, 0);
    }
}

void
VideoPage::_chatPanelResizeBarGrid__PointerEntered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    if (chtBoxOrientation == Orientation::Horizontal) {
        CoreApplication::MainView->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::SizeWestEast, 0);
    }
    else {
        CoreApplication::MainView->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::SizeNorthSouth, 0);
    }
}

void
VideoPage::_chatPanelResizeBarGrid__PointerExited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    if (!isResizingChatPanel) {
        CoreApplication::MainView->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::Arrow, 0);
    }
}

void
VideoPage::_chatPanelResizeBarGrid__PointerPressed(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    isResizingChatPanel = true;
    lastchatPanelSize = chatPanelSize;
}

void
VideoPage::_chatPanelResizeBarGrid__PointerReleased(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e)
{
    isResizingChatPanel = false;
}

void
VideoPage::_chatPanelResizeBarGrid__ManipulationDelta(Platform::Object^ sender, ManipulationDeltaRoutedEventArgs^ e)
{
    if (chtBoxOrientation == Orientation::Horizontal) {
        chatPanelSize = lastchatPanelSize - e->Cumulative.Translation.X;
    }
    else {
        chatPanelSize = lastchatPanelSize - e->Cumulative.Translation.Y;
    }
    resizeChatPanel();
}

void
VideoPage::_chatPanelResizeBarGrid__ManipulationCompleted(Platform::Object^ sender, ManipulationCompletedRoutedEventArgs^ e)
{
    isResizingChatPanel = false;
    CoreApplication::MainView->CoreWindow->PointerCursor = ref new Windows::UI::Core::CoreCursor(Windows::UI::Core::CoreCursorType::Arrow, 0);
}

void
VideoPage::openChatPanel()
{
    resizeChatPanel();
    isChatPanelOpen = true;
    SmartPanelItemsViewModel::instance->_selectedItem->_contact->_unreadMessages = 0;
}
void
VideoPage::closeChatPanel()
{
    _colChatBx_->Width = 0;
    _rowChatBx_->Height = 0;
    isChatPanelOpen = false;
}

void
VideoPage::resizeChatPanel()
{
    // clamp chatPanelSize
    double minChatPanelSize = 176;
    double maxChatPanelSize = chtBoxOrientation == Orientation::Horizontal ?
        _videoContent_->ActualWidth * .5 :
        _videoContent_->ActualHeight * .5;
    chatPanelSize = std::max(minChatPanelSize, std::min(chatPanelSize, maxChatPanelSize));

    if (chtBoxOrientation == Orientation::Horizontal) {
        _colChatBx_->Width = GridLength(chatPanelSize, GridUnitType::Pixel);
        _rowChatBx_->Height = 0;
    }
    else {
        _colChatBx_->Width = 0;
        _rowChatBx_->Height = GridLength(chatPanelSize, GridUnitType::Pixel);
    }
}

void
VideoPage::_btnToggleOrientation__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e)
{
    bool wasChatPanelOpen = isChatPanelOpen;
    closeChatPanel();

    if (chtBoxOrientation == Orientation::Horizontal) {
        chtBoxOrientation = Orientation::Vertical;
        _btnToggleOrientation_->Content = L"\uE90D";

        _chatPanelResizeBarGrid_->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Stretch;
        _chatPanelResizeBarGrid_->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Top;
        _chatPanelResizeBarGrid_->Width = std::numeric_limits<double>::quiet_NaN();
        _chatPanelResizeBarGrid_->Height = 4;

        Grid::SetColumn(_chatPanelGrid_, 0);
        Grid::SetRow(_chatPanelGrid_, 2);
    }
    else {
        chtBoxOrientation = Orientation::Horizontal;
        _btnToggleOrientation_->Content = L"\uE90E";

        _chatPanelResizeBarGrid_->HorizontalAlignment = Windows::UI::Xaml::HorizontalAlignment::Left;
        _chatPanelResizeBarGrid_->VerticalAlignment = Windows::UI::Xaml::VerticalAlignment::Stretch;
        _chatPanelResizeBarGrid_->Width = 4;
        _chatPanelResizeBarGrid_->Height = std::numeric_limits<double>::quiet_NaN();

        Grid::SetColumn(_chatPanelGrid_, 2);
        Grid::SetRow(_chatPanelGrid_, 0);
    }

    if (wasChatPanelOpen)
        openChatPanel();
}

void
VideoPage::_videoContent__SizeChanged(Platform::Object^ sender, Windows::UI::Xaml::SizeChangedEventArgs^ e)
{
    if (isChatPanelOpen)
        resizeChatPanel();
}

void
VideoPage::_messageTextBox__TextChanged(Platform::Object^ sender, TextChangedEventArgs^ e)
{
    bool carriageReturnPressed = false;
    if (_messageTextBox_->Text->Length() == (lastMessageText->Length() + 1) &&
        _messageTextBox_->Text != "\r") {
        unsigned cursorPos = 0;
        auto strMessage = Utils::toString(_messageTextBox_->Text);
        auto strLastMessage = Utils::toString(lastMessageText);
        for (std::string::size_type i = 0; i < strLastMessage.size(); ++i) {
            if (strMessage[i] != strLastMessage[i]) {
                auto changed = strMessage.substr(i, 1);
                if (changed == "\r") {
                    carriageReturnPressed = true;
                    MSG_("CR inside");
                }
                break;
            }
        }

        if (strMessage.substr(strMessage.length() - 1) == "\r") {
            if (lastMessageText->Length() != 0) {
                carriageReturnPressed = true;
                MSG_("CR at end");
            }
        }
    }

    if (carriageReturnPressed && !(RingD::instance->isCtrlPressed || RingD::instance->isShiftPressed)) {
        _messageTextBox_->Text = lastMessageText;
        sendMessage();
        lastMessageText = "";
    }

    DependencyObject^ child = VisualTreeHelper::GetChild(_messageTextBox_, 0);
    auto sendBtnElement = Utils::xaml::FindVisualChildByName(child, "_sendBtn_");
    auto sendBtn = dynamic_cast<Button^>(sendBtnElement);
    if (_messageTextBox_->Text != "") {
        sendBtn->IsEnabled = true;
    }
    else {
        sendBtn->IsEnabled = false;
    }

    lastMessageText = _messageTextBox_->Text;
}

void
VideoPage::_corewindow__KeyDown(CoreWindow^ sender, KeyEventArgs^ e)
{
    auto ctrlState = CoreWindow::GetForCurrentThread()->GetKeyState(VirtualKey::Control);
    auto isCtrlDown = ctrlState != CoreVirtualKeyStates::None;
    if (isCtrlDown && e->VirtualKey == Windows::System::VirtualKey::I) {
        _smartInfoBorder_->Visibility = _smartInfoBorder_->Visibility == VIS::Collapsed ? VIS::Visible : VIS::Collapsed;
    }
}
