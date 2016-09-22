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
    deviceList = ref new Vector<String^>();
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
                            deviceList->Append(devInfo->Name + "-Front");
                        }
                        else if (location->Panel == Windows::Devices::Enumeration::Panel::Back) {
                            deviceList->Append(devInfo->Name + "-Back");
                        }
                        else {
                            deviceList->Append(devInfo->Name);
                        }
                    }
                    else {
                        deviceList->Append(devInfo->Name);
                    }
                    WriteLine(deviceList->GetAt(i));
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

    auto devInfo = devInfoCollection->GetAt(0);

    if (devInfo == nullptr)
        return create_task([](){});

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->VideoDeviceId = devInfo->Id;

    return create_task(mediaCapture->InitializeAsync(settings))
        .then([this]()
    {
        isInitialized = true;
        WriteLine("InitializeCameraAsync DONE");
        return StartPreviewAsync();
    });
}