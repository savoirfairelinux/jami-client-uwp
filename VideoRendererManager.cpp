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

#include "VideoRendererManager.h"

using namespace RingClientUWP;
using namespace Video;

using namespace Platform;

using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel::Core;

void
VideoRenderer::bindSinkFunctions()
{
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
    auto id = Utils::toPlatformString(this->id);
    VideoManager::instance->rendererManager()->raiseWriteVideoFrame(id);
}

VideoRendererManager::VideoRendererManager()
{
    renderers = ref new Map<String^, VideoRendererWrapper^>();
}

void
VideoRendererManager::startedDecoding(String^ id, int width, int height)
{
    renderers->Insert(id, ref new VideoRendererWrapper());
    auto renderer = renderers->Lookup(id)->renderer;
    renderer->id = Utils::toString(id);
    renderer->bindSinkFunctions();
    renderer->width = width;
    renderer->height = height;
    MSG_(Utils::toString( "startedDecoding for sink id: " + id).c_str());
    registerSinkTarget(id, renderer->target);
}

void
VideoRendererManager::registerSinkTarget(String^ sinkID, const DRing::SinkTarget& target)
{
    MSG_(Utils::toString( "registerSinkTarget for sink id: " + sinkID).c_str());
    DRing::registerSinkTarget(Utils::toString(sinkID), target);
}

void
VideoRendererManager::raiseWriteVideoFrame(String^ id)
{
    auto renderer = renderers->Lookup(id)->renderer;
    if (!renderer)
        return;
    writeVideoFrame(id,
                    renderer->daemonFramePtr_->ptr,
                    renderer->width,
                    renderer->height);
}

void
VideoRendererManager::raiseClearRenderTarget()
{
    clearRenderTarget();
}

void
VideoRendererManager::removeRenderer(String^ id)
{
    if(!renderers)
        return;
    auto renderer_w = renderer(id);
    if (renderer_w) {
        std::unique_lock<std::mutex> lk(renderer_w->render_mutex);
        renderer_w->frame_cv.wait(lk, [=] {
            return !renderer_w->isRendering;
        });
        renderers->Remove(id);
    }
}

VideoRendererWrapper^
VideoRendererManager::renderer(String^ id)
{
    if(!renderers)
        return nullptr;
    return renderers->Lookup(id);
}