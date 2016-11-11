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

#include "Video.h"

using namespace RingClientUWP;
using namespace Video;

using namespace Platform;
using namespace Windows::Media::MediaProperties;

/************************************************************
 *                                                          *
 *                         Rate                             *
 *                                                          *
 ***********************************************************/
Rate::Rate()
{
    m_validFormats = ref new Vector<String^>();
}

String^
Rate::name()
{
    return m_name;
}

unsigned int
Rate::value()
{
    return m_value;
}

String^
Rate::format()
{
    return m_currentFormat;
}

Vector<String^>^
Rate::formatList()
{
    return m_validFormats;
}

IMediaEncodingProperties^
Rate::getMediaEncodingProperties()
{
    return m_encodingProperties;
}

void
Rate::setName(String^ name)
{
    m_name = name;
}

void
Rate::setValue(unsigned int value)
{
    m_value = value;
}

void
Rate::setFormat(String^ format)
{
    m_currentFormat = format;
}

void
Rate::setMediaEncodingProperties(IMediaEncodingProperties^ props)
{
    m_encodingProperties = props;
}

/************************************************************
 *                                                          *
 *                         Resolution                       *
 *                                                          *
 ***********************************************************/

Resolution::Resolution(IMediaEncodingProperties^ encodingProperties):
    m_encodingProperties(encodingProperties)
{
    m_validRates = ref new Vector<Rate^>();
    VideoEncodingProperties^ vidprops = static_cast<VideoEncodingProperties^>(encodingProperties);
    m_width = vidprops->Width;
    m_height = vidprops->Height;
}

String^
Resolution::getFriendlyName()
{
    std::wstringstream ss;
    ss << m_width << "x" << m_height;
    return ref new String(ss.str().c_str());
}

IMediaEncodingProperties^
Resolution::getMediaEncodingProperties()
{
    return m_encodingProperties;
}

String^
Resolution::name()
{
    return m_width.ToString() + "x" + m_height.ToString();
}

Rate^
Resolution::activeRate()
{
    return m_currentRate;
}

Vector<Rate^>^
Resolution::rateList()
{
    return m_validRates;
}

unsigned int
Resolution::width()
{
    return m_width;
}

unsigned int
Resolution::height()
{
    return m_height;
}

void
Resolution::setWidth(unsigned int width)
{
    m_width = width;
}

void
Resolution::setHeight(unsigned int height)
{
    m_height = height;
}

String^
Resolution::format()
{
    return m_format;
}

void
Resolution::setFormat(String^ format)
{
    m_format = format;
}

bool
Resolution::setActiveRate(Rate^ rate)
{
    if (m_currentRate == rate)
        return false;

    m_currentRate = rate;
    return true;
}

void
Resolution::setMediaEncodingProperties(IMediaEncodingProperties^ props)
{
    m_encodingProperties = props;
}

/************************************************************
 *                                                          *
 *                         Device                           *
 *                                                          *
 ***********************************************************/

Device::Device(String^ id)
{
    m_deviceId = id;
    m_validResolutions = ref new Vector<Resolution^>();
}

String^
Device::id()
{
    return m_deviceId;
}

String^
Device::name()
{
    return m_name;
}

Resolution^
Device::currentResolution()
{
    return m_currentResolution;
}

Vector<Resolution^>^
Device::resolutionList()
{
    return m_validResolutions;
}

void
Device::setName(String^ name)
{
    m_name = name;
}

void
Device::setCurrentResolution(Resolution^ currentResolution)
{
    m_currentResolution = currentResolution;
}

void
Device::save()
{
}

bool
Device::isActive()
{
    return Video::VideoManager::instance->captureManager()->activeDevice == this;
}

void
Device::SetDeviceProperties(String^ format, int width, int height, int rate)
{
    for (auto resolution_ : m_validResolutions) {
        if (    resolution_->width() == width &&
                resolution_->height() == height )
        {
            setCurrentResolution(resolution_);
            for (auto rate_ : resolution_->rateList()) {
                if (rate_->value() == rate &&
                    (format->IsEmpty()? true : rate_->format() == format))
                    currentResolution()->setActiveRate(rate_);
            }
            return;
        }
    }
}