/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: J�ger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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

using namespace Windows::UI::Xaml;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Media::Capture;
using namespace Windows::Foundation;
using namespace Concurrency;

namespace RingClientUWP
{

delegate void StartPreviewing();
delegate void StopPreviewing();
delegate Windows::UI::Xaml::Controls::CaptureElement^ GetSink();

namespace Video
{

public ref class VideoCaptureManager sealed
{
internal:
    property bool isPreviewing
    {
        bool get() { return isPreviewing_; }
        void set(bool value) { isPreviewing_ = value; }
    }

    Map<String^,String^>^ getSettings(String^ device);

    VideoCaptureManager();

    Windows::Graphics::Display::DisplayInformation^ displayInformation;
    Windows::Graphics::Display::DisplayOrientations displayOrientation;

    Windows::System::Display::DisplayRequest^ displayRequest;

    const GUID RotationKey;

    bool externalCamera;
    bool mirroringPreview;
    bool isInitialized;
    bool isChangingCamera;
    bool isRendering;

    Vector<Device^>^ deviceList;
    Device^ activeDevice;

    DeviceInformationCollection^ devInfoCollection;

    Platform::Agile<Windows::Media::Capture::MediaCapture^> mediaCapture;

    task<void> InitializeCameraAsync();
    task<void> StartPreviewAsync();
    task<void> StopPreviewAsync();
    task<void> EnumerateWebcamsAsync();
    task<void> CleanupCameraAsync();

    // event tokens
    EventRegistrationToken mediaCaptureFailedEventToken;
    EventRegistrationToken displayInformationEventToken;
    EventRegistrationToken visibilityChangedEventToken;

    cancellation_token_source* captureTaskTokenSource;

    void MediaCapture_Failed(MediaCapture ^currentCaptureObject, MediaCaptureFailedEventArgs^ errorEventArgs);
    void AddVideoDevice(uint8_t index);
    void SetCaptureSettings();

    DispatcherTimer^ videoFrameCopyInvoker;
    task<void> CopyFrameAsync();
    void CopyFrame(Object^ sender, Object^ e);
    void InitializeCopyFrameDispatcher();

    event StartPreviewing^ startPreviewing;
    event StopPreviewing^ stopPreviewing;
    event GetSink^ getSink;

private:
    bool isPreviewing_;

};

} /* namespace Video */
} /* namespace RingClientUWP */