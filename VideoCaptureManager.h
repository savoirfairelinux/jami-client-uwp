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

using namespace Windows::UI::Xaml;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::Devices::Enumeration;
using namespace Windows::Media::Capture;
using namespace Windows::Foundation;
using namespace Windows::UI::Xaml::Controls;
using namespace Concurrency;

namespace RingClientUWP
{

delegate void StartPreviewing();
delegate void StopPreviewing();
delegate CaptureElement^ GetSink();
delegate CaptureElement^ GetSettingsPreviewSink();
delegate void CaptureEnumerationComplete();

namespace Video
{

public ref class VideoCaptureManager sealed
{
public:
    double aspectRatio();

internal:
    property bool isPreviewing
    {
        bool get() { return isPreviewing_; }
        void set(bool value) { isPreviewing_ = value; }
    }

    property bool isSettingsPreviewing
    {
        bool get() { return isSettingsPreviewing_; }
        void set(bool value) { isSettingsPreviewing_ = value; }
    }

    Map<String^,String^>^ getSettings(String^ device);

    VideoCaptureManager();

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

    task<void> InitializeCameraAsync(bool isSettingsPreview);
    task<void> StartPreviewAsync(bool isSettingsPreview);
    task<void> StopPreviewAsync();
    task<void> EnumerateWebcamsAsync();
    task<void> CleanupCameraAsync();

    // event tokens
    EventRegistrationToken mediaCaptureFailedEventToken;
    EventRegistrationToken visibilityChangedEventToken;

    cancellation_token_source* captureTaskTokenSource;

    void MediaCapture_Failed(MediaCapture ^currentCaptureObject, MediaCaptureFailedEventArgs^ errorEventArgs);
    task<void> AddVideoDeviceAsync(uint8_t index);
    void SetCaptureSettings();

    DispatcherTimer^ videoFrameCopyInvoker;
    task<void> CopyFrameAsync();
    void CopyFrame(Object^ sender, Object^ e);
    void InitializeCopyFrameDispatcher(unsigned frameRate);

    event StartPreviewing^ startPreviewing;
    event StopPreviewing^ stopPreviewing;
    event GetSink^ getSink;
    event GetSettingsPreviewSink^ getSettingsPreviewSink;
    event CaptureEnumerationComplete^ captureEnumerationComplete;

private:
    bool isPreviewing_;
    bool isSettingsPreviewing_;

};

} /* namespace Video */
} /* namespace RingClientUWP */