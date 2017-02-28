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
#include <algorithm>
#include <chrono>

using namespace RingClientUWP;
using namespace RingClientUWP::Views;
using namespace Video;

using namespace Windows::UI::Core;
using namespace Windows::Graphics::Display;
using namespace Windows::Graphics::Imaging;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::Media;
using namespace Windows::Media::MediaProperties;
using namespace Windows::Media::Capture;

std::map<std::string, int> pixel_formats = {
    {"NV12", 0},
    {"MJPG", 1},
    {"RGB24",2},
    {"YUV2", 3}
};

VideoCaptureManager::VideoCaptureManager():
    mediaCapture(nullptr)
    , isInitialized(false)
    , isPreviewing_(false)
    , isSettingsPreviewing_(false)
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
    InitializeCopyFrameDispatcher(120);
    captureTaskTokenSource = new cancellation_token_source();
}

double
VideoCaptureManager::aspectRatio()
{
    auto resolution = activeDevice->currentResolution();
    return static_cast<double>(resolution->width()) / static_cast<double>(resolution->height());
}

Map<String^,String^>^
VideoCaptureManager::getSettings(String^ device)
{
    return Utils::convertMap(DRing::getSettings(Utils::toString(device)));
}

void
VideoCaptureManager::MediaCapture_Failed(Capture::MediaCapture^, Capture::MediaCaptureFailedEventArgs^ errorEventArgs)
{
    std::wstringstream ss;
    ss << "MediaCapture_Failed: 0x" << errorEventArgs->Code << ": " << errorEventArgs->Message->Data();
    auto mediaDebugString = ref new String(ss.str().c_str());
    MSG_(mediaDebugString);

    if (captureTaskTokenSource)
        captureTaskTokenSource->cancel();

    CleanupCameraAsync();
}

task<void>
VideoCaptureManager::CleanupCameraAsync()
{
    MSG_("CleanupCameraAsync");

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
VideoCaptureManager::StartPreviewAsync(bool isSettingsPreview)
{
    MSG_("StartPreviewAsync");
    displayRequest->RequestActive();

    Windows::UI::Xaml::Controls::CaptureElement^ sink;
    if (isSettingsPreview)
        sink = getSettingsPreviewSink();
    else
        sink = getSink();
    sink->Source = mediaCapture.Get();

    return create_task(mediaCapture->StartPreviewAsync())
           .then([=](task<void> previewTask)
    {
        try {
            previewTask.get();
            isPreviewing = true;
            if (isSettingsPreview)
                isSettingsPreviewing = true;
            startPreviewing();
            MSG_("StartPreviewAsync DONE");
        }
        catch (Exception ^e) {
            EXC_(e);
        }
    });
}

task<void>
VideoCaptureManager::StopPreviewAsync()
{
    MSG_("StopPreviewAsync");

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
                MSG_("StopPreviewAsync DONE");
            }
            catch (Exception ^e) {
                EXC_(e);
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
    MSG_("InitializeCameraAsync");

    if (captureTaskTokenSource)
        captureTaskTokenSource->cancel();

    mediaCapture = ref new MediaCapture();

    mediaCaptureFailedEventToken = mediaCapture->Failed +=
        ref new Capture::MediaCaptureFailedEventHandler(this, &VideoCaptureManager::MediaCapture_Failed);

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->VideoDeviceId = activeDevice->id();

    return create_task(mediaCapture->InitializeAsync(settings))
           .then([=](task<void> initTask)
    {
        try {
            initTask.get();
            SetCaptureSettings();
            isInitialized = true;
            MSG_("InitializeCameraAsync DONE");
            return StartPreviewAsync(isSettingsPreview);
        }
        catch (Exception ^e) {
            EXC_(e);
            return concurrency::task_from_result();
        }
    });
}

task<void>
VideoCaptureManager::EnumerateWebcamsAsync()
{
    devInfoCollection = nullptr;

    deviceList->Clear();

    // TODO: device monitor
    //auto watcher = DeviceInformation::CreateWatcher();

    return create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
           .then([this](task<DeviceInformationCollection^> findTask)
    {
        try {
            devInfoCollection = findTask.get();
            if (devInfoCollection == nullptr || devInfoCollection->Size == 0) {
                MSG_("No WebCams found.");
                RingD::instance->startDaemon();
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
                        RingD::instance->startDaemon();
                    }
                    catch (Exception^ e) {
                        ERR_("One doesn't simply start Ring daemon...");
                        EXC_(e);
                    }
                });
            }
        }
        catch (Platform::Exception^ e) {
            EXC_(e);
        }
    });
}

task<void>
VideoCaptureManager::AddVideoDeviceAsync(uint8_t index)
{
    MSG_("GetDeviceCaps " + index.ToString());
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
                rate->setFormat(format);
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
                    // If so, pick the best format (prefer NV12 -> MJPG -> YUV2 -> RGB24 -> scrap the rest),
                    // otherwise append to resolution's ratelist, and continue looping
                    std::vector<int>::size_type rate_index;
                    for(rate_index = 0; rate_index != matchingResolution->rateList()->Size; rate_index++) {
                        auto matchingRate = matchingResolution->rateList()->GetAt(rate_index);
                        if (matchingRate->value() == rate->value())
                            break;
                    }
                    if (rate_index < matchingResolution->rateList()->Size) {
                        // Rate found, pick best pixel-format
                        if (pixel_formats[Utils::toString(format)] <
                            pixel_formats[Utils::toString(matchingResolution->activeRate()->format())]) {
                            matchingResolution->activeRate()->setFormat(format);
                            // set props again
                            matchingResolution->activeRate()->setMediaEncodingProperties(props);
                        }
                        continue;
                    }
                    // Rate NOT found
                    device->resolutionList()->GetAt(resolution_index)->rateList()->Append(rate);
                    continue;
                }
                // Resolution NOT found, append rate to this resolution's ratelist and append resolution
                // to device's resolutionlist
                rate->setFormat(format);
                rate->setMediaEncodingProperties(props);
                resolution->rateList()->Append(rate);
                resolution->setActiveRate(rate);
                resolution->setFormat(format);
                device->resolutionList()->Append(resolution);
            }
            auto location = devInfo->EnclosureLocation;
            if (location != nullptr) {
                if (location->Panel == Windows::Devices::Enumeration::Panel::Front) {
                    device->setName(devInfo->Name + " - Front");
                }
                else if (location->Panel == Windows::Devices::Enumeration::Panel::Back) {
                    device->setName(devInfo->Name + " - Back"); // TODO: ignore these back panel cameras..?
                }
                else {
                    device->setName(devInfo->Name);
                }
            }
            else {
                device->setName(devInfo->Name);
            }
            for (auto res : device->resolutionList()) {
                for (auto rate : res->rateList()) {
                    MSG_(device->name()    + " " + res->width().ToString()     + "x"      + res->height().ToString()
                                                + " " + rate->value().ToString()    + "FPS "   + rate->format());
                }
            }
            this->deviceList->Append(device);
            this->activeDevice = deviceList->GetAt(0);
            MSG_("GetDeviceCaps DONE");

            std::vector<std::map<std::string, std::string>> devInfo;
            Vector<Video::Resolution^>^ resolutions = device->resolutionList();
            for (auto& res : resolutions) {
                for (auto& rate : res->rateList()) {
                    std::map<std::string, std::string> setting;
                    setting["format"] = Utils::toString(rate->format());
                    setting["width"] = Utils::toString(res->width().ToString());
                    setting["height"] = Utils::toString(res->height().ToString());
                    setting["rate"] = Utils::toString(rate->value().ToString());
                    devInfo.emplace_back(std::move(setting));
                    MSG_("<DeviceAdded> : info - "
                        + rate->format()
                        + ":" + res->width().ToString()
                        + "x" + res->height().ToString() + " " + rate->value().ToString()
                    );
                }
            }
            DRing::addVideoDevice(Utils::toString(device->name()), &devInfo);
        }
        catch (Platform::Exception^ e) {
            EXC_(e);
        }
    });
}

void
VideoCaptureManager::InitializeCopyFrameDispatcher(unsigned frameRate)
{
    try {
        TimeSpan timeSpan;
        timeSpan.Duration = static_cast<long long>(1e7) / frameRate;

        if (videoFrameCopyInvoker != nullptr)
            delete(videoFrameCopyInvoker);

        videoFrameCopyInvoker = ref new DispatcherTimer;
        videoFrameCopyInvoker->Interval = timeSpan;
        videoFrameCopyInvoker->Tick += ref new Windows::Foundation::EventHandler<Object^>(this, &VideoCaptureManager::CopyFrame);
        isRendering = false;
    }
    catch (Exception^ e) {
        EXC_(e);
    }
}

void
VideoCaptureManager::CopyFrame(Object^ sender, Object^ e)
{
    if (!isRendering && isPreviewing) {
        create_task(VideoCaptureManager::CopyFrameAsync())
            .then([=](task<void> copyTask)
        {
            try {
                copyTask.get();
            }
            catch (Exception^ e) {
                EXC_(e);
                isRendering = false;
                StopPreviewAsync();
                videoFrameCopyInvoker->Stop();
                if (captureTaskTokenSource)
                    captureTaskTokenSource->cancel();
                CleanupCameraAsync();
                throw ref new Exception(e->HResult, e->Message);
            }
        });
    }
}

task<void>
VideoCaptureManager::CopyFrameAsync()
{
    unsigned int videoFrameWidth = activeDevice->currentResolution()->width();
    unsigned int videoFrameHeight = activeDevice->currentResolution()->height();

   /* auto allprops = mediaCapture->VideoDeviceController->GetAvailableMediaStreamProperties(MediaStreamType::VideoPreview);
    MediaProperties::VideoEncodingProperties^ vidprops = static_cast<VideoEncodingProperties^>(allprops->GetAt(0));
    String^ format = vidprops->Subtype;
    MSG_(format);*/

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

                    BitmapBuffer^ buffer = bitmap->LockBuffer(BitmapBufferAccessMode::ReadWrite);
                    IMemoryBufferReference^ reference = buffer->CreateReference();
                    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> byteAccess;

                    if (SUCCEEDED(reinterpret_cast<IUnknown*>(reference)->QueryInterface(
                                      IID_PPV_ARGS(&byteAccess)))) {
                        byte* data;
                        unsigned capacity;
                        byteAccess->GetBuffer(&data, &capacity);
                        byte* buf = (byte*)DRing::obtainFrame(capacity);
                        if (buf)
                            std::memcpy(buf, data, static_cast<size_t>(capacity));
                        DRing::releaseFrame((void*)buf);
                    }

                    delete reference;
                    delete buffer;
                }
                delete currentFrame;

            }
            catch (Exception^ e) {
                EXC_(e);
                throw ref new Exception(e->HResult, e->Message);
            }
        }).then([=](task<void> renderCaptureToBufferTask) {
            try {
                renderCaptureToBufferTask.get();
                isRendering = false;
            }
            catch (Platform::Exception^ e) {
                EXC_(e);
            }
        });
    }
    catch(Exception^ e) {
        EXC_(e);
        throw ref new Exception(e->HResult, e->Message);
    }
}

void
VideoCaptureManager::SetCaptureSettings()
{
    MSG_("SetCaptureSettings");
    auto res = activeDevice->currentResolution();
    auto vidprops = ref new VideoEncodingProperties;
    vidprops->Width = res->width();
    vidprops->Height = res->height();
    vidprops->FrameRate->Numerator = res->activeRate()->value();
    vidprops->FrameRate->Denominator = 1;
    vidprops->Subtype = res->activeRate()->format();
    auto encodingProperties = static_cast<IMediaEncodingProperties^>(vidprops);
    create_task(mediaCapture->VideoDeviceController->SetMediaStreamPropertiesAsync(
                    MediaStreamType::VideoPreview, encodingProperties))
        .then([=](task<void> setpropsTask){
        try {
            setpropsTask.get();
            std::string deviceName = Utils::toString(activeDevice->name());
            std::map<std::string, std::string> settings = DRing::getSettings(deviceName);
            settings["name"] = Utils::toString(activeDevice->name());
            settings["rate"] = Utils::toString(res->activeRate()->value().ToString());
            settings["size"] = Utils::toString(res->getFriendlyName());
            DRing::applySettings(deviceName, settings);
            DRing::setDefaultDevice(deviceName);
            MSG_("SetCaptureSettings DONE");
        }
        catch (Exception^ e) {
            EXC_(e);

        }
    });
}