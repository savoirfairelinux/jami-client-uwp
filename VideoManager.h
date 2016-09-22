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

namespace RingClientUWP
{

namespace Video
{

ref class VideoCaptureManager;
ref class VideoRendererManager;

public ref class VideoManager sealed
{
internal:
    /* ref class singleton */
    static property VideoManager^ instance
    {
        VideoManager^ get()
        {
            static VideoManager^ instance_ = ref new VideoManager();
            return instance_;
        }
    }

public:
    VideoCaptureManager^    captureManager  ();
    VideoRendererManager^   rendererManager ();

private:
    VideoManager();

    VideoCaptureManager^    videoCaptureManager;
    VideoRendererManager^   videoRendererManager;

};

} /* namespace Video */
} /* namespace RingClientUWP */