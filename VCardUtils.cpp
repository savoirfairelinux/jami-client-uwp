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
    std::size_t start = token1 ? (str.find(token1) + strlen(token1)) : 0;
    std::size_t length = token2 ? (str.find(token2) - 1 - start) : std::string::npos;
    return str.substr(start, length);
}

VCard::VCard(Contact^ owner) : m_Owner(owner)
{}

int
VCard::receiveChunk(const std::string& args, const std::string& payload)
{
    MSG_("receive VCARD chunk");

    int32_t _id = stoi( getBetweenTokens(args, Symbols::ID_TOKEN, Symbols::PART_TOKEN) );
    uint32_t _part = stoi( getBetweenTokens(args, Symbols::PART_TOKEN, Symbols::OF_TOKEN) );
    uint32_t _of = stoi( getBetweenTokens(args, Symbols::OF_TOKEN) );

    std::stringstream _payload(payload);
    std::string _line;

    if (_part == 1) {

        m_mParts.clear();

        while (std::getline(_payload, _line)) {
            if (_line.find("UID:") != std::string::npos)
                break;
        }
        m_mParts[Property::UID] = _line.substr(4);

        while (std::getline(_payload, _line)) {
            if (_line.find("FN:") != std::string::npos)
                break;
        }
        m_mParts[Property::FN] = _line.substr(3);

        while (std::getline(_payload, _line)) {
            if (_line.find("PHOTO;") != std::string::npos)
                break;
        }
        // because android client builds vcard differently (TYPE=PNG: vs PNG:)
        m_mParts[Property::PHOTO].append(_line.substr(_line.find("PNG:") + 4));
        return VCARD_INCOMPLETE;
    }
    else {
        if (_part == _of) {
            std::getline(_payload, _line);
            m_mParts[Property::PHOTO].append(_line);
            saveToFile();
            decodeBase64ToPNGFile();
            if (!m_mParts[Property::FN].empty())
                m_Owner->_displayName = Utils::toPlatformString(m_mParts[Property::FN]);
            m_Owner->_vcardUID = Utils::toPlatformString(m_mParts[Property::UID]);
            ViewModel::ContactsViewModel::instance->saveContactsToFile();
            MSG_("VCARD_COMPLETE");
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
VCard::send(std::string callID, const char* vCardFile)
{
    int i = 0;
    const int chunkSize = 4096;
    std::string vCard;
    if (vCardFile) {
        std::ifstream file(vCardFile);
        vCard.assign(( std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    }
    else
        vCard = asString();
    int total = vCard.size() / chunkSize + (vCard.size() % chunkSize ? 1 : 0);
    std::string idkey = Utils::genID(0LL, 99999999LL);
    while ( vCard.size() ) {
        std::map<std::string, std::string> chunk;
        std::stringstream key;
        key << PROFILE_VCF
            << ";id=" << idkey
            << ",part=" << std::to_string(i + 1)
            << ",of=" << std::to_string(total);
        chunk[key.str()] = vCard.substr(0, chunkSize);
        vCard = vCard.erase(0, chunkSize);
        ++i;
        sendChunk(callID, chunk);
    }
}

void
VCard::sendChunk(std::string callID, std::map<std::string, std::string> chunk)
{
    MSG_("sending chunk...");
    for(auto i : chunk)
        MSG_("key:" + i.first + "\nvalue:" + i.second);
    RingD::instance->sendSIPTextMessageVCF(callID, chunk);
}

std::string
VCard::asString()
{
    std::stringstream ret;

    ret << Symbols::BEGIN_TOKEN     << Symbols::END_LINE_TOKEN;

    ret << Property::VERSION        << Symbols::SEPERATOR2      << "3.0"
        << Symbols::END_LINE_TOKEN;

    ret << Property::UID            << Symbols::SEPERATOR2      << m_mParts[Property::UID]
        << Symbols::END_LINE_TOKEN;

    ret << Property::FN             << Symbols::SEPERATOR2      << m_mParts[Property::FN]
        << Symbols::END_LINE_TOKEN;

    ret << Property::PHOTO          << Symbols::SEPERATOR1      << Symbols::PHOTO_ENC
        << Symbols::SEPERATOR1      << Symbols::PHOTO_TYPE      << Symbols::SEPERATOR2
        << m_mParts[Property::PHOTO]                            << Symbols::END_LINE_TOKEN;

    ret << Symbols::END_TOKEN;

    return std::string(ret.str());
}

void
VCard::decodeBase64ToPNGFile()
{
    size_t padding = m_mParts[Property::PHOTO].size() % 4;
    if (padding)
        m_mParts[Property::PHOTO].append(padding, 0);

    std::vector<uint8_t> decodedData = ring::base64::decode(m_mParts[Property::PHOTO]);
    std::string vcardDir(RingD::instance->getLocalFolder() + ".vcards\\");
    std::string pngFile(m_mParts[Property::UID] + ".png");
    MSG_("Saving PNG (" + m_mParts[Property::UID] + ") to " + pngFile + "...");
    if (_mkdir(vcardDir.c_str())) {
        std::ofstream file(vcardDir + pngFile, std::ios::out | std::ios::trunc | std::ios::binary);
        if (file.is_open()) {
            for (auto i : decodedData)
                file << i;
            file.close();
            m_Owner->_avatarImage = Utils::toPlatformString(vcardDir + pngFile);
            MSG_("Done decoding and saving VCard Photo to PNG");
        }
    }
}

void
VCard::encodePNGToBase64()
{
    std::string vcardDir(RingD::instance->getLocalFolder() + ".vcards\\");
    std::string pngFile(m_mParts[Property::UID] + ".png");
    std::basic_ifstream<uint8_t> file(vcardDir + pngFile, std::ios::in | std::ios::binary);
    if (file.is_open()) {
        auto eos = std::istreambuf_iterator<uint8_t>();
        auto data = std::vector<uint8_t>(std::istreambuf_iterator<uint8_t>(file), eos);
        m_mParts[Property::PHOTO] = ring::base64::encode(data);
        file.close();
        MSG_("Done encoding PNG to b64");
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
        if (file.is_open()) {
            file << asString();
            file.close();
            MSG_("Done Saving VCard");
            return 1;
        }
    }
    return 0;
}

void
VCard::setData(std::map<std::string, std::string> data)
{
    m_mParts = data;
}