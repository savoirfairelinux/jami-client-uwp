/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
* Author: Traczyk Andreas <traczyk.andreas@savoirfairelinux.com>          *
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

/* https://www.ietf.org/rfc/rfc2045.txt
 * https://www.ietf.org/rfc/rfc2047.txt
 * https://www.ietf.org/rfc/rfc2426.txt
 */

//BEGIN:VCARD
//VERSION:3.0
//N:Gump;Forrest
//FN:Forrest Gump
//ORG:Bubba Gump Shrimp Co.
//TITLE:Shrimp Man
//PHOTO;VALUE=URL;TYPE=GIF:http://www.example.com/dir_photos/my_photo.gif
//TEL;TYPE=WORK,VOICE:(111) 555-1212
//TEL;TYPE=HOME,VOICE:(404) 555-1212
//ADR;TYPE=WORK:;;100 Waters Edge;Baytown;LA;30314;United States of America
//LABEL;TYPE=WORK:100 Waters Edge\nBaytown, LA 30314\nUnited States of America
//ADR;TYPE=HOME:;;42 Plantation St.;Baytown;LA;30314;United States of America
//LABEL;TYPE=HOME:42 Plantation St.\nBaytown, LA 30314\nUnited States of America
//EMAIL;TYPE=PREF,INTERNET:forrestgump@example.com
//REV:20080424T195243Z
//END:VCARD

#define VCARD_INCOMPLETE    0
#define VCARD_COMPLETE      1
#define VCARD_CHUNK_ERROR   -1

namespace RingClientUWP
{
namespace VCardUtils
{

const std::string PROFILE_VCF = "x-ring/ring.profile.vcard";

struct Symbols {
    constexpr static const char* BEGIN_TOKEN            =   "BEGIN:VCARD";
    constexpr static const char* END_TOKEN              =   "END:VCARD";
    constexpr static const char* END_LINE_TOKEN         =   "\n";
    constexpr static const char* ID_TOKEN               =   "id=";
    constexpr static const char* PART_TOKEN             =   "part=";
    constexpr static const char* OF_TOKEN               =   "of=";
    constexpr static const char* SEPERATOR1             =   ";";
    constexpr static const char* SEPERATOR2             =   ":";
    constexpr static const char* PHOTO_ENC              =   "ENDCODING=BASE64";
    constexpr static const char* PHOTO_TYPE             =   "TYPE=PNG";
};

struct Property {
    constexpr static const char* UID                    =   "UID";
    constexpr static const char* VERSION                =   "VERSION";
    constexpr static const char* ADDRESS                =   "ADR";
    constexpr static const char* AGENT                  =   "AGENT";
    constexpr static const char* BIRTHDAY               =   "BDAY";
    constexpr static const char* CATEGORIES             =   "CATEGORIES";
    constexpr static const char* CLASS                  =   "CLASS";
    constexpr static const char* DELIVERY_LABEL         =   "LABEL";
    constexpr static const char* EMAIL                  =   "EMAIL";
    constexpr static const char* FORMATTED_NAME         =   "FN";
    constexpr static const char* GEOGRAPHIC_POSITION    =   "GEO";
    constexpr static const char* KEY                    =   "KEY";
    constexpr static const char* LOGO                   =   "LOGO";
    constexpr static const char* MAILER                 =   "MAILER";
    constexpr static const char* NAME                   =   "N";
    constexpr static const char* NICKNAME               =   "NICKNAME";
    constexpr static const char* NOTE                   =   "NOTE";
    constexpr static const char* ORGANIZATION           =   "ORG";
    constexpr static const char* PHOTO                  =   "PHOTO";
    constexpr static const char* PRODUCT_IDENTIFIER     =   "PRODID";
    constexpr static const char* REVISION               =   "REV";
    constexpr static const char* ROLE                   =   "ROLE";
    constexpr static const char* SORT_STRING            =   "SORT-STRING";
    constexpr static const char* SOUND                  =   "SOUND";
    constexpr static const char* TELEPHONE              =   "TEL";
    constexpr static const char* TIME_ZONE              =   "TZ";
    constexpr static const char* TITLE                  =   "TITLE";
    constexpr static const char* URL                    =   "URL";

    constexpr static const char* X_RINGACCOUNT          =   "X-RINGACCOUNTID";
};

using namespace std;

using ProfileChunk = string;

class VCard
{
public:
    int                     receiveChunk(   const std::string& args,
                                            const std::string& payload);
    int                     saveToFile();
    void                    base64DecodeToFile();

private:
    unordered_map<int, string>              m_hParts     {       };
    int                                     m_PartsCount { 0     };
    vector<ProfileChunk>                    m_lChunks;
    map<string, string>                     m_mParts     {       };
};

}
}