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

#include "VideoRenderManager.h"

using namespace RingClientUWP;
using namespace Video;

using namespace Platform;

// dispatcher ( DEBUG )
using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Core;

Resolution::Resolution()
{
    m_Size = ref new Video::Size();
}

Resolution::Resolution(Video::Size^ size):
    m_Size(size)
{ }

String^
Resolution::name()
{
   return size()->width.ToString() + "x" + size()->height.ToString();
}

String^
Resolution::activeRate()
{
    return m_CurrentRate;
}

Video::Size^
Resolution::size()
{
    return m_Size;
}

void
Resolution::setWidth(int width)
{
   m_Size->width = width;
}

void
Resolution::setHeight(int height)
{
   m_Size->height = height;
}

bool
Resolution::setActiveRate(String^ rate)
{
   if (m_CurrentRate == rate)
      return false;

   m_CurrentRate = rate;
   // set camera device rate here
   return true;
}

void
VideoRenderer::bindSinkFunctions()
{
    MSG_("binding sink functions");
    using namespace std::placeholders;
    target.pull = std::bind(&VideoRenderer::requestFrameBuffer,this, _1);
    target.push = std::bind(&VideoRenderer::onNewFrame,this, _1);
}

DRing::SinkTarget::FrameBufferPtr
VideoRenderer::requestFrameBuffer(std::size_t bytes)
{
    std::lock_guard<std::mutex> lk(video_mutex);
    if (!daemonFramePtr_)
        daemonFramePtr_.reset(new DRing::FrameBuffer);
    daemonFramePtr_->storage.resize(bytes);
    daemonFramePtr_->ptr = daemonFramePtr_->storage.data();
    daemonFramePtr_->ptrSize = bytes;
    return std::move(daemonFramePtr_);
}

void
VideoRenderer::onNewFrame(DRing::SinkTarget::FrameBufferPtr buf)
{
    std::lock_guard<std::mutex> lk(video_mutex);
    daemonFramePtr_ = std::move(buf);
    VideoRenderManager::instance->raiseWriteVideoFrame();
}

VideoRenderManager::VideoRenderManager()
{
    videoRenderer = new VideoRenderer;
    videoRenderer->bindSinkFunctions();
}

void
VideoRenderManager::startedDecoding(String^ id, int width, int height)
{
    incomingResolution.setWidth(width);
    incomingResolution.setHeight(height);
    MSG_(Utils::toString( "startedDecoding for sink id: " + id).c_str());
    registerSinkTarget(id, videoRenderer->target);
}

void
VideoRenderManager::stopPreview()
{
    MSG_("stopPreview");
    // stop camera preview
    isPreviewing_ = false;
}

void
VideoRenderManager::startPreview()
{
    MSG_("startPreview");
    // start camera preview
    isPreviewing_ = true;
}

void
VideoRenderManager::registerSinkTarget(String^ sinkID, const DRing::SinkTarget& target)
{
    MSG_(Utils::toString( "registerSinkTarget for sink id: " + sinkID).c_str());
    DRing::registerSinkTarget(Utils::toString(sinkID), target);
}

void
VideoRenderManager::raiseWriteVideoFrame()
{
    writeVideoFrame(videoRenderer->daemonFramePtr_->ptr,
                    incomingResolution.size()->width,
                    incomingResolution.size()->height);
}