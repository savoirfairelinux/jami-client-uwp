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

delegate void WriteVideoFrame(uint8_t*, int, int);

namespace Video
{
public ref class Size sealed
{
internal:
    int width;
    int height;
public:
    Size() { };
    Size(int w, int h): width(w), height(h) { };
    Size(Size^ obj) : width(obj->width), height(obj->height) { };
};

public ref class Resolution sealed
{
public:
    Resolution();
    Resolution(Size^ size);

    String^              name            ();
    String^              activeRate      ();
    Size^                size            ();

    bool setActiveRate   (   String^ rate    );
    void setWidth        (   int     width   );
    void setHeight       (   int     height  );

private:
    String^             m_CurrentRate;
    Size^               m_Size;

};

class VideoRenderer
{
    static VideoRenderer *m_instance;

public:
    /* singleton */
    static VideoRenderer *instance()
    {
        if (!m_instance)
            m_instance = new VideoRenderer;
        return m_instance;
    }

    std::mutex video_mutex;
    DRing::SinkTarget target;
    DRing::SinkTarget::FrameBufferPtr daemonFramePtr_;

    void bindSinkFunctions();

    DRing::SinkTarget::FrameBufferPtr requestFrameBuffer(std::size_t bytes);
    void onNewFrame(DRing::SinkTarget::FrameBufferPtr buf);
};

public ref class VideoRenderManager sealed
{
internal:
    /* ref class singleton */
    static property VideoRenderManager^ instance
    {
        VideoRenderManager^ get()
        {
            static VideoRenderManager^ instance_ = ref new VideoRenderManager();
            return instance_;
        }
    }

    property bool isPreviewing
    {
        bool get() { return isPreviewing_; }
        void set(bool value) { isPreviewing_ = value; }
    }

    void startedDecoding(String^ id, int width, int height);
    void registerSinkTarget(String^ sinkID, const DRing::SinkTarget& target);
    void stopPreview();
    void startPreview();

    /* events */
    event WriteVideoFrame^ writeVideoFrame;

public:
    void raiseWriteVideoFrame();

private:
    VideoRenderManager();

    Resolution incomingResolution;

    VideoRenderer* videoRenderer;
    bool isPreviewing_;
};
}
}