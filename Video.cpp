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

int
Video::Size::width()
{
    return m_Width;
}

int
Video::Size::height()
{
    return m_Height;
}

void
Video::Size::setWidth(int width)
{
    m_Width = width;
}

void
Video::Size::setHeight(int height)
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

Resolution^
Rate::resolution()
{
    return m_Resolution;
}

void
Rate::setName(String^ name)
{
    m_name = name;
}

void
Rate::setResolution(Resolution^ resolution)
{
    m_Resolution = resolution;
}

/************************************************************
 *                                                          *
 *                         Channel                          *
 *                                                          *
 ***********************************************************/

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

Device^
Channel::device()
{
    return m_device;
}

/************************************************************
 *                                                          *
 *                         Resolution                       *
 *                                                          *
 ***********************************************************/

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
   return size()->width().ToString() + "x" + size()->height().ToString();
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
    m_Size->setWidth(width);
}

void
Resolution::setHeight(int height)
{
    m_Size->setHeight(height);
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

/************************************************************
 *                                                          *
 *                         Device                           *
 *                                                          *
 ***********************************************************/

Device::Device(String^ id)
{
    m_deviceId = id;

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
    // TOFU
    return nullptr;
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
Device::save()
{

}

bool
Device::isActive()
{
    return false;
   //return Video::DeviceModel::instance().activeDevice() == this;
}