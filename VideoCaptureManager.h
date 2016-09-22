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

namespace RingClientUWP
{

namespace Video
{

ref class VideoPreviewManager;

public ref class VideoCaptureManager sealed
{
internal:
    Map<String^,String^>^ getSettings(String^ device);

    VideoCaptureManager();

    VideoPreviewManager^ previewManager;

    Windows::Graphics::Display::DisplayInformation^ displayInformation;
    Windows::Graphics::Display::DisplayOrientations displayOrientation;

    Windows::System::Display::DisplayRequest^ displayRequest;

    const GUID RotationKey;

    bool externalCamera;
    bool mirroringPreview;
    bool isInitialized;
    bool isPreviewing;
    bool isChangingCamera;
    bool isRendering;

    Vector<String^>^ deviceList;

    DeviceInformationCollection^ m_devInfoCollection;

    Platform::Agile<Windows::Media::Capture::MediaCapture^> mediaCapture;

    void ChangeDevice();

    Concurrency::task<void> EnumerateWebcamsAsync();
    Concurrency::task<Windows::Devices::Enumeration::DeviceInformation^>
        FindCameraDeviceByPanelAsync(Windows::Devices::Enumeration::Panel panel);

};

} /* namespace Video */
} /* namespace RingClientUWP */