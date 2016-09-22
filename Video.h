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

using namespace Platform;

namespace RingClientUWP
{

namespace Video
{

ref class Rate;
ref class Resolution;
ref class Device;

public ref class Size sealed
{
internal:
    int                         width           ();
    int                         height          ();

    void setWidth               (   int    width   );
    void setHeight              (   int    height  );

public:
    Size() { };
    Size(int w, int h):
        m_Width(w),
        m_Height(h) { };
    Size(Size^ rhs):
        m_Width(rhs->m_Width),
        m_Height(rhs->m_Height) { };

private:
    int m_Width;
    int m_Height;

};

public ref class Rate sealed
{
internal:
    String^                     name            ();
    Resolution^                 resolution      ();

    void setName                (   String^     name        );
    void setResolution          (   Resolution^ resolution  );

private:
    String^                     m_name;
    Resolution^                 m_Resolution;

};

public ref class Channel sealed
{
internal:
    String^                     name                ();
    Resolution^                 currentResolution   ();

    void setName                (   String^     name             );
    void setCurrentResolution   (   Resolution^ currentResolution);

    Vector<Resolution^>^        resolutionList  ();
    Device^                     device          ();

private:
    String^                     m_name;
    Resolution^                 m_currentResolution;
    Vector<Resolution^>^        m_validResolutions;
    Device^                     m_device;

};

public ref class Resolution sealed
{
internal:
    String^                     name            ();
    String^                     activeRate      ();
    Size^                       size            ();

    bool setActiveRate          (   String^ rate    );
    void setWidth               (   int     width   );
    void setHeight              (   int     height  );

public:
    Resolution();
    Resolution(Size^ size);

private:
    String^                     m_CurrentRate;
    Size^                       m_Size;

};

public ref class Device sealed
{
internal:
    class PreferenceNames {
    public:
        constexpr static const char* RATE    = "rate"   ;
        constexpr static const char* NAME    = "name"   ;
        constexpr static const char* CHANNEL = "channel";
        constexpr static const char* SIZE    = "size"   ;
    };

    Vector<Channel^>^           channelList     ();
    String^                     id              ();
    String^                     name            ();

    bool setCurrentChannel      (   Channel^ channel );

public:
    Device(String^ id);

    void save                   ();
    bool isActive               ();

private:
    String^                     m_deviceId        ;
    Channel^                    m_currentChannel  ;
    Vector<Channel^>^           m_channels        ;
    bool                        m_requireSave     ;

};

} /* namespace Video */
} /* namespace RingClientUWP */