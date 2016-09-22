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
    , RotationKey({ 0xC380465D, 0x2271, 0x428C,{ 0x9B, 0x83, 0xEC, 0xEA, 0x3B, 0x4A, 0x85, 0xC1 } })
{
    deviceList = ref new Vector<Device^>();
    InitializeCopyFrameDispatcher();
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
                    AddVideoDevice(i);
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

    if (mediaCapture.Get()) {
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
    else {
        return create_task([](){});
    }
}

task<void>
VideoCaptureManager::InitializeCameraAsync()
{
    WriteLine("InitializeCameraAsync");

    mediaCapture = ref new MediaCapture();

    auto devInfo = devInfoCollection->GetAt(0); //preferenced video capture device

    if (devInfo == nullptr)
        return create_task([](){});

    //SetCaptureSettings();

    auto settings = ref new MediaCaptureInitializationSettings();
    settings->VideoDeviceId = devInfo->Id;

    return create_task(mediaCapture->InitializeAsync(settings))
        .then([this](task<void> initTask)
    {
        try {
            initTask.get();
            SetCaptureSettings();
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
                unsigned int frame_rate = 0;
                if (vidprops->FrameRate->Denominator != 0)
                    frame_rate = vidprops->FrameRate->Numerator / vidprops->FrameRate->Denominator;
                rate->setValue(frame_rate);
                rate->setName(rate->value().ToString() + "fps");
                resolution->setActiveRate(rate);
                String^ format = vidprops->Subtype;
                resolution->setFormat(format);
                channel->resolutionList()->Append(resolution);
                WriteLine(devInfo->Name + " "
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
                    device->setName(devInfo->Name + "-Back");
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
        WriteLine(e->ToString());
    }
}

void
VideoCaptureManager::CopyFrame(Object^ sender, Object^ e)
{
    try {
        //WriteLine("CopyFrame");
        auto previewProperties = static_cast<MediaProperties::VideoEncodingProperties^>(
            mediaCapture->VideoDeviceController->GetMediaStreamProperties(Capture::MediaStreamType::VideoPreview));
        unsigned int videoFrameWidth = previewProperties->Width;
        unsigned int videoFrameHeight = previewProperties->Height;
        unsigned int videoFrameRate = previewProperties->FrameRate->Numerator;

        auto videoFrame = ref new VideoFrame(BitmapPixelFormat::Bgra8, videoFrameWidth, videoFrameHeight);

        isRendering = true;
        create_task(mediaCapture->GetPreviewFrameAsync(videoFrame))
            .then([this,videoFrameRate](VideoFrame^ currentFrame)
        {
            try {
                auto bitmap = currentFrame->SoftwareBitmap;
                if (bitmap->BitmapPixelFormat == BitmapPixelFormat::Bgra8)
                {
                    const int BYTES_PER_PIXEL = 4;

                    BitmapBuffer^ buffer = bitmap->LockBuffer(BitmapBufferAccessMode::ReadWrite);
                    IMemoryBufferReference^ reference = buffer->CreateReference();

                    Microsoft::WRL::ComPtr<IMemoryBufferByteAccess> byteAccess;
                    if (SUCCEEDED(reinterpret_cast<IUnknown*>(reference)->QueryInterface(
                        IID_PPV_ARGS(&byteAccess))))
                    {
                        byte* data;
                        unsigned capacity;
                        byteAccess->GetBuffer(&data, &capacity);

                        auto desc = buffer->GetPlaneDescription(0);

                        uint8_t* buf = (uint8_t*)DRing::obtainFrame(capacity);

                        for (int row = 0; row < desc.Height; row++)
                        {
                            for (int col = 0; col < desc.Width; col++)
                            {
                                auto currPixel = desc.StartIndex + desc.Stride * row
                                    + BYTES_PER_PIXEL * col;
                                buf[currPixel + 0] = data[currPixel + 0];
                                buf[currPixel + 1] = data[currPixel + 1];
                                buf[currPixel + 2] = data[currPixel + 2];
                            }
                        }

                        DRing::releaseFrame((void*)buf);

                        //WriteLine( desc.Width.ToString() + "x" + desc.Height.ToString() + "x4" );
                    }
                    delete reference;
                    delete buffer;
                }
                if (currentFrame) {
                    delete currentFrame;
                    //WriteLine("CopyFrame DONE " + n.ToString());
                }
            }
            catch (Exception^ e) {
                WriteException(e);
            }
        });
    }
    catch(Exception^ e) {
        WriteException(e);
    }
}

void
VideoCaptureManager::SetCaptureSettings()
{
    //MediaProperties::VideoEncodingProperties^ vidprops = static_cast<VideoEncodingProperties^>(props);
    /*int width = vidprops->Width;
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
    WriteLine(devInfo->Name + " "
        + width.ToString() + "x" + height.ToString()
        + " " + frame_rate.ToString() + "FPS" + " " + format);*/
    auto vp = ref new VideoEncodingProperties;
    auto res = activeDevice->channel()->currentResolution();
    vp->Width = res->size()->width();
    vp->Height = res->size()->height();
    vp->FrameRate->Numerator = res->activeRate()->value();
    vp->FrameRate->Denominator = 1;
    vp->Subtype = res->format();
    /*for (auto encodingProperties : allProperties) {
        static_cast<VideoEncodingProperties^>(encodingProperties)->Width;
    }*/
    //WriteLine("");
    auto encodingProperties = static_cast<IMediaEncodingProperties^>(vp);
    create_task(mediaCapture->VideoDeviceController->SetMediaStreamPropertiesAsync(
        MediaStreamType::VideoPreview, encodingProperties));
}