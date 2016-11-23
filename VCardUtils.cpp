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

using namespace RingClientUWP;
using namespace VCard;

int
VCardUtils::addChunk(std::map<std::string, std::string> args, std::string payload)
{
    const int total         = stoi( args[ "of"   ]);
    const int part          = stoi( args[ "part" ]);
    const std::string id    =       args[ "id"   ];

    if (part < 1 || part > total)
        return VCARD_CHUNK_ERROR;

    if (!m_PartsCount)
        m_PartsCount = total;

    m_hParts[part] = payload;
    m_lChunks.emplace_back(m_hParts[part]);

    if (m_hParts.size() == m_PartsCount) {
        saveToFile();
        return VCARD_COMPLETE;
    }

    return VCARD_INCOMPLETE;
}

void
VCardUtils::saveToFile()
{
    // TODO: use UID
    std::string vcardFile(Utils::toString("UID" + ".vcard"));

    std::ofstream file(vcardFile);
    if (file.is_open())
    {
        for (const auto& chunk : m_lChunks) {
            file << chunk;
        }
        file.close();
    }
}