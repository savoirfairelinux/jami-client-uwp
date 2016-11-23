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
#include <pch.h>

#include "base64.h"

#include <direct.h>

using namespace RingClientUWP;
using namespace VCardUtils;

using namespace Windows::UI::Core;

std::string
getBetweenTokens(   const std::string& str,
                    const char* token1,
                    const char* token2 = nullptr)
{
    std::size_t start = str.find(token1) + strlen(token1);
    std::size_t length = token2 ? (str.find(token2) - 1 - start) : std::string::npos;
    return str.substr(start, length);
}

int
VCard::receiveChunk(const std::string& args, const std::string& payload)
{
    MSG_("VCARD chunk");

    int32_t _id = stoi( getBetweenTokens(args, Symbols::ID_TOKEN, Symbols::PART_TOKEN) );
    uint32_t _part = stoi( getBetweenTokens(args, Symbols::PART_TOKEN, Symbols::OF_TOKEN) );
    uint32_t _of = stoi( getBetweenTokens(args, Symbols::OF_TOKEN) );

    if (_part == 1) {
        std::stringstream payload(payload);
        std::string line;

        while (std::getline(payload, line)) {
            if (line.find("UID:") != std::string::npos)
                break;
        }

        m_mParts[Property::UID] = line.substr(4);

        while (std::getline(payload, line)) {
            if (line.find("PHOTO;") != std::string::npos)
                break;
        }
        m_mParts[Property::PHOTO].append(line.substr(26));
        return VCARD_INCOMPLETE;
    }
    else {
        if (_part == _of) {
            MSG_("VCARD_COMPLETE");
            m_mParts[Property::PHOTO].append(payload);
            saveToFile();
            base64DecodeToFile();
            return VCARD_COMPLETE;
        }
        else {
            m_mParts[Property::PHOTO].append(payload);
            return VCARD_INCOMPLETE;
        }
    }
    return VCARD_CHUNK_ERROR;
}

void
VCard::base64DecodeToFile()
{
    //std::vector<uint8_t> decodedData = ring::base64::decode(m_mParts[Property::PHOTO]);
    std::string decodedData = Utils::base64_decode(m_mParts[Property::PHOTO]);
    std::string encodedData = Utils::base64_encode((const unsigned char*)decodedData.c_str(),decodedData.length());
    std::string vcardDir(RingD::instance->getLocalFolder() + ".vcards\\");
    std::string pngFile(m_mParts[Property::UID] + ".png");
    std::string encpngFile(m_mParts[Property::UID] + "_enc.txt");
    MSG_("Saving PNG (" + m_mParts[Property::UID] + ") to " + pngFile + "...");
    if (_mkdir(vcardDir.c_str())) {
        std::ofstream file(vcardDir + pngFile, std::ios::out | std::ios::trunc | ios_base::binary);
        if (file.is_open())
        {
            file << decodedData;
            //for (auto i : decodedData)
            //    file << i;
            file.close();
            MSG_("Done Saving VCard PNG");
        }
        std::ofstream efile(vcardDir + encpngFile, std::ios::out | std::ios::trunc | ios_base::binary);
        if (efile.is_open())
        {
            efile << encodedData;
            //for (auto i : decodedData)
            //    file << i;
            efile.close();
            MSG_("Done Saving b64Enc VCard PNG");
        }
    }
}

int
VCard::saveToFile()
{
    std::string vcardDir(RingD::instance->getLocalFolder() + ".vcards\\");
    std::string vcardFile(m_mParts[Property::UID] + ".vcard");
    MSG_("Saving VCard (" + m_mParts[Property::UID] + ") to " + vcardFile + "...");
    if (_mkdir(vcardDir.c_str())) {
        std::ofstream file(vcardDir + vcardFile, std::ios::out | std::ios::trunc);
        if (file.is_open())
        {
            file << Symbols::BEGIN_TOKEN    << Symbols::END_LINE_TOKEN;

            file << Property::UID           << Symbols::SEPERATOR2      << m_mParts[Property::UID]
                 << Symbols::END_LINE_TOKEN;

            file << Property::PHOTO         << Symbols::SEPERATOR1      << Symbols::PHOTO_ENC
                 << Symbols::SEPERATOR1     << Symbols::PHOTO_TYPE      << Symbols::SEPERATOR2
                 << m_mParts[Property::PHOTO]                           << Symbols::END_LINE_TOKEN;

            file << Symbols::END_TOKEN;
            file.close();
            MSG_("Done Saving VCard");
            return 1;
        }
    }
    return 0;
}