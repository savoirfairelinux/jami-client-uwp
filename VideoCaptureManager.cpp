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

#include "VideoCaptureManager.h"

#include <MemoryBuffer.h>   // IMemoryBufferByteAccess

using namespace RingClientUWP;
using namespace Video;

using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Media;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;

VideoCaptureManager::VideoCaptureManager():
    mediaCapture(nullptr)
    , isInitialized(false)
    , isPreviewing_(false)
    , isChangingCamera(false)
    , isRendering(false)
    , externalCamera(false)
    , mirroringPreview(false)
    , displayOrientation(DisplayOrientations::Portrait)
    , displayRequest(ref new Windows::System::Display::DisplayRequest())
    , RotationKey( {
    0xC380465D, 0x2271, 0x428C, { 0x9B, 0x83, 0xEC, 0xEA, 0x3B, 0x4A, 0x85, 0xC1 }
})
{
    deviceList = ref new Vector<Device^>();
    InitializeCopyFrameDispatcher();
    captureTaskTokenSource = new cancellation_token_source();
}

Map<String^,String^>^
VideoCaptureManager::getSettings(String^ device)
{
    return Utils::convertMap(DRing::getSettings(Utils::toString(device)));
}

void
VideoCaptureManager::MediaCapture_Failed(Capture::MediaCapture^, Capture::MediaCaptureFailedEventArgs^ errorEventArgs)
{
    RingDebug::instance->WriteLine("MediaCapture_Failed");
    std::wstringstream ss;
    ss << "MediaCapture_Failed: 0x" << errorEventArgs->Code << ": " << errorEventArgs->Message->Data();
    RingDebug::instance->WriteLine(ref new String(ss.str().c_str()));

    if (captureTaskTokenSource)
        captureTaskTokenSource->cancel();
    CleanupCameraAsync();
}

task<void>
VideoCaptureManager::CleanupCameraAsync()
{
    RingDebug::instance->WriteLine("CleanupCameraAsync");

    std::vector<task<void>> taskList;

    if (isInitialized)
    {
        if (isPreviewing)
        {
            auto stopPreviewTask = create_task(StopPreviewAsync());
            taskList.push_back(stopPreviewTask);
        }

        isInitialized = false;
    }

    return when_all(taskList.begin(), taskList.end())
           .then([this]()
    {
        if (mediaCapture.Get() != nullptr)
        {
            mediaCapture->Failed -= mediaCaptureFailedEventToken;
            mediaCapture = nullptr;
        }
    });
}

task<void>
VideoCaptureManager::EnumerateWebcamsAsync()
{
    devInfoCollection = nullptr;

    deviceList->Clear();

    return create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
           .then([this](task<DeviceInformationCollection^> findTask)
    {
        try {
            devInfoCollection = findTask.get();
            if (devInfoCollection == nullptr || devInfoCollection->Size == 0) {
                RingDebug::instance->WriteLine("No WebCams found.");
            }
            else {
                for (unsigned int i = 0; i < devInfoCollection->Size; i++) {
                    AddVideoDevice(i);
                }
                RingDebug::instance->WriteLine("Enumerating Webcams completed successfully.");
            }
        }
        catch (Platform::Exception^ e) {
            WriteException(e);
        }
    });
}

task<void>
VideoCaptureManager::StartPreviewAsync()
{
    RingDebug::instance->RingDebug::instance->WriteLine("StartPreviewAsync");
    displayRequest->RequestActive();

    auto sink = getSink();
    sink->Source = mediaCapture.Get();

    return create_task(mediaCapture->StartPreviewAsync())
           .then([this](task<void> previewTask)
    {
        try {
            previewTask.get();
            isPreviewing = true;
            startPreviewing();
            RingDebug::instance->WriteLine("StartPreviewAsync DONE");
        }
        catch (Exception ^e) {
            WriteException(e);
        }
    });
}

task<void>
VideoCaptureManager::StopPreviewAsync()
{
    RingDebug::instance->WriteLine("StopPreviewAsync");

    if (captureTaskTokenSource)
        captureTaskTokenSource->cancel();

    if (mediaCapture.Get()) {
        return create_task(mediaCapture->StopPreviewAsync())
               .then([this](task<void> stopTask)
        {
            try {
                stopTask.get();
                isPreviewing = false;
                stopPreviewing();
                displayRequest->RequestRelease();
                RingDebug::instance->WriteLine("StopPreviewAsync DONE");
            }
            catch (Exception ^e) {
                WriteException(e);
            }
        });
    }
    else {
        return create_task([]() {});
    }
}

task<void>
VideoCaptureManager::InitializeCameraAsync()
{
    RingDebug::instance->WriteLine("InitializeCameraAsync");

    if (captureTaskTokenSource)
        captureTaskTokenSource->cancel();

    mediaCapture = ref new MediaCapture();

    auto devInfo = devInfoCollection->GetAt(0); //preferences - video capture device

    mediaCaptureFailedEventToken = mediaCapture->Failed +=
                                       ref new Capture::MediaCaptureFailedEventHandler(this, &VideoCaptureManager::MediaCapture_Failed);

    if (devInfo == nullptr)
        return create_task([]() {});

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->VideoDeviceId = devInfo->Id;

    return create_task(mediaCapture->InitializeAsync(settings))
           .then([this](task<void> initTask)
    {
        try {
            initTask.get();
            SetCaptureSettings();
            isInitialized = true;
            RingDebug::instance->WriteLine("InitializeCameraAsync DONE");
            return StartPreviewAsync();
        }
        catch (Exception ^e) {
            WriteException(e);
            return create_task([]() {});
        }
    });
}

void
VideoCaptureManager::AddVideoDevice(uint8_t index)
{
    RingDebug::instance->WriteLine("GetDeviceCaps " + index.ToString());
    Platform::Agile<Windows::Media::Capture::MediaCapture^> mc;
    mc = ref new MediaCapture();

    auto devInfo = devInfoCollection->GetAt(index);

    if (devInfo == nullptr)
        return;

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->VideoDeviceId = devInfo->Id;

    create_task(mc->InitializeAsync(settings))
    .then([=](task<void> initTask)
    {
        try {
            initTask.get();
            auto allprops = mc->VideoDeviceController->GetAvailableMediaStreamProperties(MediaStreamType::VideoPreview);
            Video::Device^ device = ref new Device(devInfo->Id);
            Video::Channel^ channel = ref new Channel();
            for (auto props : allprops) {
                MediaProperties::VideoEncodingProperties^ vidprops = static_cast<VideoEncodingProperties^>(props);
                int width = vidprops->Width;
                int height = vidprops->Height;
                Video::Resolution^ resolution = ref new Resolution(ref new Size(width,height));
                Video::Rate^ rate = ref new Rate();
                unsigned int frame_rate = 0;
                if (vidprops->FrameRate->Denominator != 0)
                    frame_rate = vidprops->FrameRate->Numerator / vidprops->FrameRate->Denominator;
                rate->setValue(frame_rate);
                rate->setName(rate->value().ToString() + "fps");
                resolution->setActiveRate(rate);
                String^ format = vidprops->Subtype;
                resolution->setFormat(format);
                channel->resolutionList()->Append(resolution);
                RingDebug::instance->WriteLine(devInfo->Name + " "
                                               + width.ToString() + "x" + height.ToString()
                                               + " " + frame_rate.ToString() + "FPS" + " " + format);
            }
            device->channelList()->Append(channel);
            device->setCurrentChannel(device->channelList()->GetAt(0));
            auto location = devInfo->EnclosureLocation;
            if (location != nullptr) {
                if (location->Panel == Windows::Devices::Enumeration::Panel::Front) {
                    device->setName(devInfo->Name + "-Front");
                }
                else if (location->Panel == Windows::Devices::Enumeration::Panel::Back) {
                    device->setName(devInfo->Name + "-Back"); //ignore
                }
                else {
                    device->setName(devInfo->Name);
                }
            }
            else {
                device->setName(devInfo->Name);
            }
            this->deviceList->Append(device);
            this->activeDevice = deviceList->GetAt(0);
            RingDebug::instance->WriteLine("GetDeviceCaps DONE");
            DRing::addVideoDevice(Utils::toString(device->name()));
        }
        catch (Platform::Exception^ e) {
            WriteException(e);
        }
    });
}

void
VideoCaptureManager::InitializeCopyFrameDispatcher()
{
    try {
        TimeSpan timeSpan;
        timeSpan.Duration = static_cast<long long>(1e7) / 30; // framerate

        if (videoFrameCopyInvoker != nullptr)
            delete(videoFrameCopyInvoker);

        videoFrameCopyInvoker = ref new DispatcherTimer;
        videoFrameCopyInvoker->Interval = timeSpan;
        videoFrameCopyInvoker->Tick += ref new Windows::Foundation::EventHandler<Object^>(this, &VideoCaptureManager::CopyFrame);
        isRendering = false;
    }
    catch (Exception^ e) {
        RingDebug::instance->WriteLine(e->ToString());
    }
}

void
VideoCaptureManager::CopyFrame(Object^ sender, Object^ e)
{
    if (!isRendering && isPreviewing) {
        try {
            create_task(VideoCaptureManager::CopyFrameAsync());
        }
        catch(Platform::COMException^ e) {
            RingDebug::instance->WriteLine(e->ToString());
        }
    }
}

task<void>
VideoCaptureManager::CopyFrameAsync()
{
    unsigned int videoFrameWidth = activeDevice->channel()->currentResolution()->size()->width();
    unsigned int videoFrameHeight = activeDevice->channel()->currentResolution()->size()->height();

    // for now, only bgra
    auto videoFrame = ref new VideoFrame(BitmapPixelFormat::Bgra8, videoFrameWidth, videoFrameHeight);

    try {
        if (captureTaskTokenSource) {
            delete captureTaskTokenSource;
            captureTaskTokenSource = new cancellation_token_source();
        }
        return create_task(mediaCapture->GetPreviewFrameAsync(videoFrame), captureTaskTokenSource->get_token())
               .then([this](VideoFrame^ currentFrame)
        {
            try {
                isRendering = true;
                auto bitmap = currentFrame->SoftwareBitmap;
                if (bitmap->BitmapPixelFormat == BitmapPixelFormat::Bgra8) {
                    const int BYTES_PER_PIXEL = 4;

                    BitmapBuffer^ buffer = bitmap->LockBuffer(BitmapBufferAccessMode::ReadWrite);
                    IMemoryBufferReference^ reference = buffer->CreateReference();

                    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> byteAccess;
                    if (SUCCEEDED(reinterpret_cast<IUnknown*>(reference)->QueryInterface(
                                      IID_PPV_ARGS(&byteAccess)))) {
                        byte* data;
                        unsigned capacity;
                        byteAccess->GetBuffer(&data, &capacity);

                        auto desc = buffer->GetPlaneDescription(0);

                        byte* buf = (byte*)DRing::obtainFrame(capacity);

                        if (buf) {
                            for (int row = 0; row < desc.Height; row++) {
                                for (int col = 0; col < desc.Width; col++) {
                                    auto currPixel = desc.StartIndex + desc.Stride * row
                                                     + BYTES_PER_PIXEL * col;
                                    buf[currPixel + 0] = data[currPixel + 0];
                                    buf[currPixel + 1] = data[currPixel + 1];
                                    buf[currPixel + 2] = data[currPixel + 2];
                                }
                            }
                        }

                        DRing::releaseFrame((void*)buf);
                    }
                    delete reference;
                    delete buffer;
                }
                delete currentFrame;

            }
            catch (Exception^ e) {
                RingDebug::instance->WriteLine("failed to copy frame to daemon's buffer");
            }
        }).then([=](task<void> previousTask) {
            try {
                previousTask.get();
                isRendering = false;
            }
            catch (Platform::Exception^ e) {
                RingDebug::instance->WriteLine( "Caught exception from previous task.\n" );
            }
        });
    }
    catch(Exception^ e) {
        WriteException(e);
        throw std::exception();
    }
}

void
VideoCaptureManager::SetCaptureSettings()
{
    auto vp = ref new VideoEncodingProperties;
    auto res = activeDevice->channel()->currentResolution();
    vp->Width = res->size()->width();
    vp->Height = res->size()->height();
    vp->FrameRate->Numerator = res->activeRate()->value();
    vp->FrameRate->Denominator = 1;
    vp->Subtype = res->format();
    auto encodingProperties = static_cast<IMediaEncodingProperties^>(vp);
    create_task(mediaCapture->VideoDeviceController->SetMediaStreamPropertiesAsync(
                    MediaStreamType::VideoPreview, encodingProperties));
}