/**************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
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

#include "../fileutils.h"

namespace RingClientUWP
{
namespace Utils
{

std::string
fileNameOnly(const std::string& path)
{
    return path.substr(path.find_last_of("\\") + 1);
}

inline int
fileExists(const std::string& name)
{
    std::ifstream f(name.c_str());
    return f.good();
}

inline int
fileDelete(const std::string& file)
{
    return std::remove(file.c_str());
}

std::string
getStringFromFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>()));
}

} /*namespace Utils*/

} /*namespace RingClientUWP*/
