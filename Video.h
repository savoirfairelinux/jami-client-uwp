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
    double                      value           ();

    void setName                (   String^     name        );
    void setValue               (   double      value       );

private:
    String^                     m_name;
    double                      m_value;

};

public ref class Channel sealed
{
internal:
    String^                     name                ();
    Resolution^                 currentResolution   ();
    Vector<Resolution^>^        resolutionList      ();

    void setName                (   String^     name             );
    void setCurrentResolution   (   Resolution^ currentResolution);

public:
    Channel();

private:
    String^                     m_name;
    Resolution^                 m_currentResolution;
    Vector<Resolution^>^        m_validResolutions;

};

public ref class Resolution sealed
{
internal:
    String^                     name            ();
    Rate^                       activeRate      ();
    Vector<Rate^>^              rateList        ();
    Size^                       size            ();
    String^                     format          ();

    bool setActiveRate          (   Rate^   rate    );
    void setWidth               (   int     width   );
    void setHeight              (   int     height  );
    void setFormat              (   String^ format  );

public:
    Resolution();
    Resolution(Size^ size);

private:
    Rate^                       m_currentRate;
    Vector<Rate^>^              m_validRates;
    Size^                       m_size;
    String^                     m_format;

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

    bool setCurrentChannel      (   Channel^ channel    );
    void setName                (   String^ name        );

public:
    Device(String^ id);

    void save                   ();
    bool isActive               ();

private:
    String^                     m_deviceId        ;
    String^                     m_name            ;
    Channel^                    m_currentChannel  ;
    Vector<Channel^>^           m_channels        ;
    bool                        m_requireSave     ;

};

} /* namespace Video */
} /* namespace RingClientUWP */