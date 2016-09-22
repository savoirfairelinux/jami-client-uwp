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

#include "videomanager_interface.h"

namespace RingClientUWP
{

delegate void WriteVideoFrame(String^, uint8_t*, int, int);
delegate void ClearRenderTarget();

namespace Video
{

class VideoRenderer
{
public:
    std::string id;
    std::mutex video_mutex;
    DRing::SinkTarget target;
    DRing::SinkTarget::FrameBufferPtr daemonFramePtr_;
    int width;
    int height;

    void bindSinkFunctions();

    DRing::SinkTarget::FrameBufferPtr requestFrameBuffer(std::size_t bytes);
    void onNewFrame(DRing::SinkTarget::FrameBufferPtr buf);

};

ref class VideoRendererWrapper sealed
{
internal:
    VideoRendererWrapper() {
        renderer = std::make_shared<VideoRenderer>();
        isRendering = false;
    };

    std::mutex render_mutex;
    std::condition_variable frame_cv;

    std::shared_ptr<VideoRenderer> renderer;
    bool isRendering;
};

public ref class VideoRendererManager sealed
{
internal:
    void startedDecoding(String^ id, int width, int height);
    void registerSinkTarget(String^ sinkID, const DRing::SinkTarget& target);

    /* events */
    event WriteVideoFrame^ writeVideoFrame;
    event ClearRenderTarget^ clearRenderTarget;

    VideoRendererWrapper^ renderer(String^ id);
    Map<String^,VideoRendererWrapper^>^ renderers;

public:
    VideoRendererManager();

    void raiseWriteVideoFrame(String^ id);
    void raiseClearRenderTarget();

    void removeRenderer(String^ id);

private:

};

} /* namespace Video */
} /* namespace RingClientUWP */