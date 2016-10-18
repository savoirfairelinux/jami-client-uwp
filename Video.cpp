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

/************************************************************
 *                                                          *
 *                         Size                             *
 *                                                          *
 ***********************************************************/

unsigned int
Video::Size::width()
{
    return m_Width;
}

unsigned int
Video::Size::height()
{
    return m_Height;
}

void
Video::Size::setWidth(unsigned int width)
{
    m_Width = width;
}

void
Video::Size::setHeight(unsigned int height)
{
    m_Height = height;
}

/************************************************************
 *                                                          *
 *                         Rate                             *
 *                                                          *
 ***********************************************************/

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

/************************************************************
 *                                                          *
 *                         Channel                          *
 *                                                          *
 ***********************************************************/
Channel::Channel()
{
    m_validResolutions = ref new Vector<Resolution^>();
}

String^
Channel::name()
{
    return m_name;
}

Resolution^
Channel::currentResolution()
{
    return m_currentResolution;
}

void
Channel::setName(String^ name)
{
    m_name = name;
}

void
Channel::setCurrentResolution(Resolution^ currentResolution)
{
    m_currentResolution = currentResolution;
}

Vector<Resolution^>^
Channel::resolutionList()
{
    return m_validResolutions;
}

/************************************************************
 *                                                          *
 *                         Resolution                       *
 *                                                          *
 ***********************************************************/

Resolution::Resolution()
{
    m_size = ref new Size();
    m_validRates = ref new Vector<Rate^>();
}

Resolution::Resolution(Video::Size^ size):
    m_size(size)
{ }

String^
Resolution::name()
{
    return size()->width().ToString() + "x" + size()->height().ToString();
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

Video::Size^
Resolution::size()
{
    return m_size;
}

String^
Resolution::format()
{
    return m_format;
}

void
Resolution::setWidth(int width)
{
    m_size->setWidth(width);
}

void
Resolution::setHeight(int height)
{
    m_size->setHeight(height);
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
    // set camera device rate here
    return true;
}

/************************************************************
 *                                                          *
 *                         Device                           *
 *                                                          *
 ***********************************************************/

Device::Device(String^ id)
{
    m_deviceId = id;
    m_channels = ref new Vector<Channel^>();
}

String^
Device::id()
{
    return m_deviceId;
}

Vector<Channel^>^
Device::channelList()
{
    return m_channels;
}

String^
Device::name()
{
    return m_name;
}

Channel^
Device::channel()
{
    return m_currentChannel;
}

bool
Device::setCurrentChannel(Channel^ channel)
{
    if (m_currentChannel == channel)
        return false;
    m_currentChannel = channel;
    return true;
}

void
Device::setName(String^ name)
{
    m_name = name;
}

void
Device::save()
{
}

bool
Device::isActive()
{
    return false;
    //return Video::DeviceModel::instance().activeDevice() == this;
}

void
Device::SetDeviceProperties(String^ format, int width, int height, int rate)
{
    auto rl = m_currentChannel->resolutionList();
    for (auto res : rl) {
        if (res->format() == format &&
                res->size()->width() == width &&
                res->size()->height() == height &&
                res->activeRate()->value() == rate)
        {
            m_currentChannel->setCurrentResolution(res);
            RingDebug::instance->WriteLine("SetDeviceProperties");
            return;
        }
    }
}