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

#define VCARD_INCOMPLETE    0
#define VCARD_COMPLETE      1
#define VCARD_CHUNK_ERROR   -1

using namespace Platform;

namespace RingClientUWP
{

ref class Contact;

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
    constexpr static const char* FN                     =   "FN";
    constexpr static const char* PHOTO                  =   "PHOTO";
    constexpr static const char* X_RINGACCOUNT          =   "X-RINGACCOUNTID";
};

public ref class VCard sealed
{
public:
    VCard(Contact^ owner, String^ accountId);

internal:
    int                     receiveChunk(const std::string& args, const std::string& payload);
    void                    sendChunk(std::string callID, std::map<std::string, std::string> chunk);
    void                    send(std::string callID, const char* file = nullptr);
    std::string             asString();
    std::string             getPart(const std::string& part);
    int                     saveToFile();
    void                    decodeBase64ToPNGFile();
    void                    encodePNGToBase64();

    void                    completeReception();
    int                     parseFromString();
    void                    setData(std::map<std::string, std::string> data);
    void                    setData(const std::string& data);

private:
    std::string             m_data;
    std::map<std::string, std::string>  m_mParts;
    Contact^                m_Owner;
    int                     m_type;
    std::string             m_accountId;
};

std::map<std::string, std::string> parseContactRequestPayload(const std::string& payload);

}
}