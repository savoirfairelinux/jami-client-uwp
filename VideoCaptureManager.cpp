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
    , RotationKey({ 0xC380465D, 0x2271, 0x428C,{ 0x9B, 0x83, 0xEC, 0xEA, 0x3B, 0x4A, 0x85, 0xC1 } })
{
    deviceList = ref new Vector<Device^>();
    deviceNameList = ref new Vector<String^>();
}

Map<String^,String^>^
VideoCaptureManager::getSettings(String^ device)
{
    return Utils::convertMap(DRing::getSettings(Utils::toString(device)));
}

task<void>
VideoCaptureManager::EnumerateWebcamsAsync()
{
    devInfoCollection = nullptr;

    deviceList->Clear();
    deviceNameList->Clear();

    return create_task(DeviceInformation::FindAllAsync(DeviceClass::VideoCapture))
        .then([this](task<DeviceInformationCollection^> findTask)
    {
        try {
            devInfoCollection = findTask.get();
            if (devInfoCollection == nullptr || devInfoCollection->Size == 0) {
                WriteLine("No WebCams found.");
            }
            else {
                for (unsigned int i = 0; i < devInfoCollection->Size; i++) {
                    auto devInfo = devInfoCollection->GetAt(i);
                    auto location = devInfo->EnclosureLocation;
                    if (location != nullptr) {
                        if (location->Panel == Windows::Devices::Enumeration::Panel::Front) {
                            deviceNameList->Append(devInfo->Name + "-Front");
                        }
                        else if (location->Panel == Windows::Devices::Enumeration::Panel::Back) {
                            deviceNameList->Append(devInfo->Name + "-Back");
                        }
                        else {
                            deviceNameList->Append(devInfo->Name);
                        }
                    }
                    else {
                        deviceNameList->Append(devInfo->Name);
                    }
                    AddVideoDevice(i);
                    WriteLine(deviceNameList->GetAt(i));
                }
                WriteLine("Enumerating Webcams completed successfully.");
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
    WriteLine("StartPreviewAsync");
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
    WriteLine("StopPreviewAsync");

    return create_task(mediaCapture->StopPreviewAsync())
        .then([this](task<void> stopTask)
    {
        try {
            stopTask.get();
            isPreviewing = false;
            stopPreviewing();
            displayRequest->RequestRelease();
            WriteLine("StopPreviewAsync DONE");
        }
        catch (Exception ^e) {
            WriteException(e);
        }
    });
}

task<void>
VideoCaptureManager::InitializeCameraAsync()
{
    WriteLine("InitializeCameraAsync");

    mediaCapture = ref new MediaCapture();

    auto devInfo = devInfoCollection->GetAt(0); //preferenced video capture device

    if (devInfo == nullptr)
        return create_task([](){});

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->VideoDeviceId = devInfo->Id;

    return create_task(mediaCapture->InitializeAsync(settings))
        .then([this](task<void> initTask)
    {
        try {
            initTask.get();
            isInitialized = true;
            WriteLine("InitializeCameraAsync DONE");
            return StartPreviewAsync();
        }
        catch (Exception ^e) {
            WriteException(e);
            return create_task([](){});
        }
    });
}

void
VideoCaptureManager::AddVideoDevice(uint8_t index)
{
    WriteLine("GetDeviceCaps " + index.ToString());
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
                double frame_rate = 0;
                if (vidprops->FrameRate->Denominator != 0)
                    frame_rate = vidprops->FrameRate->Numerator / vidprops->FrameRate->Denominator;
                rate->setValue(0);
                rate->setName(rate->value().ToString() + "fps");
                resolution->setActiveRate(rate);
                String^ format = vidprops->Subtype;
                resolution->setFormat(format);
                channel->resolutionList()->Append(resolution);
                device->channelList()->Append(channel);
                WriteLine(devInfo->Name + " "
                    + width.ToString() + "x" + height.ToString()
                    + " " + frame_rate.ToString() + "FPS" + " " + format);
            }
            device->setName(devInfo->Name);
            this->deviceList->Append(device);
            WriteLine("GetDeviceCaps DONE");
            DRing::addVideoDevice(Utils::toString(devInfo->Name));
        }
        catch (Platform::Exception^ e) {
            WriteException(e);
        }
    });
}