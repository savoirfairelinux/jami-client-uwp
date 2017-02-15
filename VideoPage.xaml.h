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

using namespace Windows::Media::Capture;
using namespace Windows::UI::Xaml::Navigation;

using namespace Windows::UI::Xaml;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Devices::Enumeration;


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

    property bool barFading
    {
        bool get()
        {
            return barFading_;
        }
        void set(bool value)
        {
            barFading_ = value;
        }
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

    void _sendBtn__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _messageTextBox__KeyDown(Platform::Object^ sender, Windows::UI::Xaml::Input::KeyRoutedEventArgs^ e);
    void sendMessage();

    Concurrency::task<void> WriteFrameAsSoftwareBitmapAsync(String^ id, uint8_t* buf, int width, int height);

    void Button_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _btnCancel__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void _btnHangUp__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnPause__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnChat__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnAddFriend__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnSwitch__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnMicrophone__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnMemo__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnHQ__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _btnVideo__Tapped(Platform::Object^ sender, Windows::UI::Xaml::Input::TappedRoutedEventArgs^ e);
    void _videoControl__PointerMoved(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void btnAny_entered(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void btnAny_exited(Platform::Object^ sender, Windows::UI::Xaml::Input::PointerRoutedEventArgs^ e);
    void OnincomingMessage(Platform::String ^callId, Platform::String ^payload);
    void _btnVideo__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void OnincomingVideoMuted(Platform::String ^callId, bool state);
    void OnstartPreviewing();
    void OnstopPreviewing();
    void _btnMicrophone__Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
    void OnaudioMuted(const std::string &callId, bool state);
    void OnvideoMuted(const std::string &callId, bool state);
    void IncomingVideoImage_DoubleTapped(Platform::Object^ sender, Windows::UI::Xaml::Input::DoubleTappedRoutedEventArgs^ e);

    // For transforming the preview image
    void InitManipulationTransforms();

    void PreviewImage_ManipulationDelta(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationDeltaRoutedEventArgs^ e);
    void PreviewImage_ManipulationCompleted(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationCompletedRoutedEventArgs^ e);
    void PreviewImageResizer_ManipulationDelta(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationDeltaRoutedEventArgs^ e);
    void PreviewImageResizer_ManipulationCompleted(Platform::Object^ sender, Windows::UI::Xaml::Input::ManipulationCompletedRoutedEventArgs^ e);

    Windows::UI::Xaml::Media::TransformGroup^ PreviewImage_transforms;
    Windows::UI::Xaml::Media::MatrixTransform^ PreviewImage_previousTransform;
    Windows::UI::Xaml::Media::CompositeTransform^ PreviewImage_deltaTransform;

};
}
}