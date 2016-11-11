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
#include "SmartPanel.xaml.h"

#include <MemoryBuffer.h>   // IMemoryBufferByteAccess

using namespace RingClientUWP;
using namespace RingClientUWP::Views;
using namespace Video;

using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Media;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;

//Map<String^, int> pixel_formats = {{"YUV2", 0}, {"NV12", 2}, {"MJPG", 3}, {"RGB24", 4}};
std::map<std::string, int> pixel_formats = {{"YUV2", 0}, {"NV12", 2}, {"MJPG", 3}, {"RGB24", 4}};

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
                WriteLine("No WebCams found.");
            }
            else {
                std::vector<task<void>> taskList;
                for (unsigned int i = 0; i < devInfoCollection->Size; i++) {
                    taskList.push_back(AddVideoDeviceAsync(i));
                }
                when_all(taskList.begin(), taskList.end())
                    .then([this](task<void> previousTasks)
                {
                    try {
                        previousTasks.get();
                        captureEnumerationComplete();
                    }
                    catch (Exception^ e) {
                        WriteException(e);
                    }
                });
            }
        }
        catch (Platform::Exception^ e) {
            WriteException(e);
        }
    });
}

task<void>
VideoCaptureManager::StartPreviewAsync(bool isSettingsPreview)
{
    WriteLine("StartPreviewAsync");
    displayRequest->RequestActive();

    Windows::UI::Xaml::Controls::CaptureElement^ sink;
    if (isSettingsPreview)
        sink = getSettingsPreviewSink();
    else
        sink = getSink();
    sink->Source = mediaCapture.Get();

    return create_task(mediaCapture->StartPreviewAsync())
           .then([this](task<void> previewTask)
    {
        try {
            previewTask.get();
            isPreviewing = true;
            startPreviewing();
            WriteLine("StartPreviewAsync DONE");
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
VideoCaptureManager::InitializeCameraAsync(bool isSettingsPreview)
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
           .then([=](task<void> initTask)
    {
        try {
            initTask.get();
            SetCaptureSettings();
            isInitialized = true;
            RingDebug::instance->WriteLine("InitializeCameraAsync DONE");
            return StartPreviewAsync(isSettingsPreview);
        }
        catch (Exception ^e) {
            WriteException(e);
            return create_task([]() {});
        }
    });
}

task<void>
VideoCaptureManager::AddVideoDeviceAsync(uint8_t index)
{
    RingDebug::instance->WriteLine("GetDeviceCaps " + index.ToString());
    Platform::Agile<Windows::Media::Capture::MediaCapture^> mc;
    mc = ref new MediaCapture();

    auto devInfo = devInfoCollection->GetAt(index);

    if (devInfo == nullptr)
        return concurrency::task_from_result();

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->VideoDeviceId = devInfo->Id;

    return create_task(mc->InitializeAsync(settings))
    .then([=](task<void> initTask)
    {
        try {
            initTask.get();
            auto allprops = mc->VideoDeviceController->GetAvailableMediaStreamProperties(MediaStreamType::VideoPreview);
            Video::Device^ device = ref new Device(devInfo->Id);
            for (auto props : allprops) {
                MediaProperties::VideoEncodingProperties^ vidprops = static_cast<VideoEncodingProperties^>(props);
                // Create resolution
                Video::Resolution^ resolution = ref new Resolution(props);
                // Get pixel-format
                String^ format = vidprops->Subtype;
                // Create rate
                Video::Rate^ rate = ref new Rate();
                unsigned int frame_rate = 0;
                if (vidprops->FrameRate->Denominator != 0)
                    frame_rate = vidprops->FrameRate->Numerator / vidprops->FrameRate->Denominator;
                rate->setValue(frame_rate);
                rate->setName(rate->value().ToString() + " FPS");
                // Try to find a resolution with the same dimensions in this device's resolution list
                std::vector<int>::size_type resolution_index;
                Video::Resolution^ matchingResolution;
                for(resolution_index = 0; resolution_index != device->resolutionList()->Size; resolution_index++) {
                    matchingResolution = device->resolutionList()->GetAt(resolution_index);
                    if (matchingResolution->width() == resolution->width() &&
                        matchingResolution->height() == resolution->height())
                        break;
                }
                if (resolution_index < device->resolutionList()->Size) {
                    // Resolution found, check if rate is already in this resolution's ratelist,
                    // If so, pick the best format (prefer YUV2 -> NV12 -> MJPG -> RGB24 -> scrap the rest),
                    // otherwise append to resolution's ratelist, and continue looping
                    std::vector<int>::size_type rate_index;
                    for(rate_index = 0; rate_index != matchingResolution->rateList()->Size; rate_index++) {
                        auto rat = matchingResolution->rateList()->GetAt(rate_index);
                        if (rat->value() == rate->value())
                            break;
                    }
                    if (rate_index < matchingResolution->rateList()->Size) {
                        // Rate found, prefer better pixel-format
                        /*if (pixel_formats.Lookup(format) < pixel_formats.Lookup(res->format))
                            res->format = format;*/
                        if (pixel_formats[Utils::toString(format)] <
                            pixel_formats[Utils::toString(matchingResolution->format())])
                            matchingResolution->setFormat(format);
                        continue;
                    }
                    // Rate NOT found
                    device->resolutionList()->GetAt(resolution_index)->rateList()->Append(rate);
                    continue;
                }
                // Resolution NOT found, append rate to this resolution's ratelist and append resolution
                // to device's resolutionlist
                resolution->rateList()->Append(rate);
                resolution->setActiveRate(rate);
                resolution->setFormat(format);
                device->resolutionList()->Append(resolution);
                WriteLine(devInfo->Name + " "
                                               + vidprops->Width.ToString() + "x" + vidprops->Height.ToString()
                                               + " " + frame_rate.ToString() + "FPS" + " " + format);
            }
            auto location = devInfo->EnclosureLocation;
            if (location != nullptr) {
                if (location->Panel == Windows::Devices::Enumeration::Panel::Front) {
                    device->setName(devInfo->Name + "-Front");
                }
                else if (location->Panel == Windows::Devices::Enumeration::Panel::Back) {
                    device->setName(devInfo->Name + "-Back"); // TODO: ignore these back panel cameras..?
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
            WriteLine("GetDeviceCaps DONE");
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
    unsigned int videoFrameWidth = activeDevice->currentResolution()->width();
    unsigned int videoFrameHeight = activeDevice->currentResolution()->height();

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
                RingDebug::instance->WriteLine("Failed to copy frame to daemon's buffer");
            }
        }).then([=](task<void> previousTask) {
            try {
                previousTask.get();
                isRendering = false;
            }
            catch (Platform::Exception^ e) {
                WriteException(e);
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
    WriteLine("SetCaptureSettings");
    auto vp = ref new VideoEncodingProperties;
    auto res = activeDevice->currentResolution();
    vp->Width = res->width();
    vp->Height = res->height();
    vp->FrameRate->Numerator = res->activeRate()->value();
    vp->FrameRate->Denominator = 1;
    vp->Subtype = res->format();
    auto encodingProperties = static_cast<IMediaEncodingProperties^>(vp);
    create_task(mediaCapture->VideoDeviceController->SetMediaStreamPropertiesAsync(
                    MediaStreamType::VideoPreview, encodingProperties))
        .then([](task<void> setpropsTask){
        try {
            setpropsTask.get();
            WriteLine("SetCaptureSettings DONE");
        }
        catch (Exception^ e) {
            WriteException(e);
        }
    });
}