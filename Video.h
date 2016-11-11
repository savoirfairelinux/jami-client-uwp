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
using namespace Windows::Media::MediaProperties;

namespace RingClientUWP
{

namespace Video
{

ref class Rate;
ref class Resolution;
ref class Device;

public ref class Rate sealed
{
internal:
    String^                     name                        ();
    unsigned int                value                       ();
    Vector<String^>^            formatList                  ();
    String^                     format                      ();
    IMediaEncodingProperties^   getMediaEncodingProperties  ();

    void setName                (   String^         name    );
    void setValue               (   unsigned int    value   );
    void setFormat              (   String^         format  );

    void setMediaEncodingProperties  (IMediaEncodingProperties^ props);

public:
    Rate();

private:
    IMediaEncodingProperties^   m_encodingProperties;
    String^                     m_name;
    unsigned int                m_value;
    String^                     m_currentFormat;
    Vector<String^>^            m_validFormats;

};

public ref class Resolution sealed
{
internal:
    String^                     name                        ();
    Rate^                       activeRate                  ();
    Vector<Rate^>^              rateList                    ();
    unsigned int                width                       ();
    unsigned int                height                      ();
    String^                     format                      ();
    String^                     getFriendlyName             ();
    IMediaEncodingProperties^   getMediaEncodingProperties  ();

    bool setActiveRate          (   Rate^           rate    );
    void setWidth               (   unsigned int    width   );
    void setHeight              (   unsigned int    height  );
    void setFormat              (   String^         format  );

    void setMediaEncodingProperties  (IMediaEncodingProperties^ props);

public:
    Resolution(IMediaEncodingProperties^ encodingProperties);

private:
    IMediaEncodingProperties^   m_encodingProperties;
    Rate^                       m_currentRate;
    Vector<Rate^>^              m_validRates;
    unsigned int                m_width;
    unsigned int                m_height;
    String^                     m_format;

};

public ref class Device sealed
{
internal:
    class PreferenceNames {
    public:
        constexpr static const char* RATE    = "rate"   ;
        constexpr static const char* NAME    = "name"   ;
        constexpr static const char* SIZE    = "size"   ;
    };

    String^                     id                  ();
    String^                     name                ();
    Resolution^                 currentResolution   ();
    Vector<Resolution^>^        resolutionList      ();

    void setName                (   String^ name                    );
    void setCurrentResolution   (   Resolution^ currentResolution   );

public:
    Device(String^ id);
    void SetDeviceProperties(String^ format, int width, int height, int rate);

    void save                   ();
    bool isActive               ();

private:
    String^                     m_deviceId          ;
    String^                     m_name              ;
    bool                        m_requireSave       ;
    Resolution^                 m_currentResolution ;
    Vector<Resolution^>^        m_validResolutions  ;

};

} /* namespace Video */
} /* namespace RingClientUWP */