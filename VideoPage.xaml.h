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

#include "VideoPage.g.h"
#include "MessageTextPage.xaml.h"

using namespace Platform;
using namespace Concurrency;
using namespace Windows::Media::Capture;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Devices::Enumeration;
using namespace Windows::UI::Xaml::Input;

namespace RingClientUWP
{
/* delegate */
delegate void PressHangUpCall();
delegate void PauseCall();
delegate void ChatPanelCall(); // nobody use this ?
delegate void AddContactCall();
delegate void TransferCall();
delegate void SwitchMicrophoneStateCall();
delegate void SwitchVideoStateCall();
delegate void ReccordVideoCall();
delegate void QualityVideoLevelCall();

namespace Views
{

public ref class VideoPage sealed
{
public:
    VideoPage();
    void updatePageContent();

    property bool barFading {
        bool get(){ return barFading_; }
        void set(bool value) { barFading_ = value; }
    }

    void scrollDown();
    void screenVideo(bool state);

protected:
    virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;

internal:
    /* events */
    event PressHangUpCall^ pressHangUpCall;
    event PauseCall^ pauseCall;
    event ChatPanelCall^ chatPanelCall;
    event AddContactCall^ addContactCall;
    event TransferCall^ transferCall;
    event SwitchMicrophoneStateCall^ switchMicrophoneStateCall;
    event SwitchVideoStateCall^ switchVideoStateCall;
    event ReccordVideoCall^ reccordVideoCall;
    event QualityVideoLevelCall^ qualityVideoLevelCall;

private:
    bool barFading_;

    // For transforming the preview image
    double userPreviewHeightModifier = 0.0;
    bool isResizingPreview = false;
    int lastQuadrant = 0;
    int quadrant = 0;

    bool chatBoxOpen = false;
    double chatBoxSize = 1;
    enum class Orientation {
        Horizontal,
        Vertical
    } chtBoxOrientation = Orientation::Vertical;

    TransformGroup^ PreviewImage_transforms;
    MatrixTransform^ PreviewImage_previousTransform;
    CompositeTransform^ PreviewImage_deltaTransform;

    Concurrency::task<void> WriteFrameAsSoftwareBitmapAsync(String^ id, uint8_t* buf, int width, int height);
    void _sendBtn__Click(Platform::Object^ sender, RoutedEventArgs^ e);
    void _messageTextBox__KeyDown(Object^ sender, KeyRoutedEventArgs^ e);
    void sendMessage();
    void Button_Click(Object^ sender, RoutedEventArgs^ e);
    void _btnCancel__Click(Object^ sender, RoutedEventArgs^ e);
    void _btnHangUp__Tapped(Object^ sender, TappedRoutedEventArgs^ e);
    void _btnPause__Tapped(Object^ sender, TappedRoutedEventArgs^ e);
    void _btnChat__Tapped(Object^ sender, TappedRoutedEventArgs^ e);
    void _btnAddFriend__Tapped(Object^ sender, TappedRoutedEventArgs^ e);
    void _btnSwitch__Tapped(Object^ sender, TappedRoutedEventArgs^ e);
    void _btnMicrophone__Tapped(Object^ sender, TappedRoutedEventArgs^ e);
    void _btnMemo__Tapped(Object^ sender, TappedRoutedEventArgs^ e);
    void _btnHQ__Tapped(Object^ sender, TappedRoutedEventArgs^ e);
    void _btnVideo__Tapped(Object^ sender, TappedRoutedEventArgs^ e);
    void _videoControl__PointerMoved(Object^ sender, PointerRoutedEventArgs^ e);
    void btnAny_entered(Object^ sender, PointerRoutedEventArgs^ e);
    void btnAny_exited(Object^ sender, PointerRoutedEventArgs^ e);
    void _btnVideo__Click(Object^ sender, RoutedEventArgs^ e);
    void _btnMicrophone__Click(Object^ sender, RoutedEventArgs^ e);
    void IncomingVideoImage_DoubleTapped(Object^ sender, DoubleTappedRoutedEventArgs^ e);
    void OnincomingMessage(String ^callId, String ^payload);
    void OnincomingVideoMuted(String ^callId, bool state);
    void OnstartPreviewing();
    void OnstopPreviewing();
    void OnaudioMuted(const std::string &callId, bool state);
    void OnvideoMuted(const std::string &callId, bool state);
    void openChatPanel();
    void closeChatPanel();
    void resizeChatPanel();
    void computeQuadrant();
    void arrangeResizer();
    void anchorPreview();
    void updatePreviewFrameDimensions();
    void InitManipulationTransforms();
    void PreviewImage_ManipulationDelta(Object^ sender, ManipulationDeltaRoutedEventArgs^ e);
    void PreviewImage_ManipulationCompleted(Object^ sender, ManipulationCompletedRoutedEventArgs^ e);
    void PreviewImageResizer_ManipulationDelta(Object^ sender, ManipulationDeltaRoutedEventArgs^ e);
    void PreviewImageResizer_ManipulationCompleted(Object^ sender, ManipulationCompletedRoutedEventArgs^ e);
    void PreviewImage_PointerReleased(Object^ sender, PointerRoutedEventArgs^ e);
    void PreviewImageResizer_PointerEntered(Object^ sender, PointerRoutedEventArgs^ e);
    void PreviewImageResizer_PointerExited(Object^ sender, PointerRoutedEventArgs^ e);
};
}
}