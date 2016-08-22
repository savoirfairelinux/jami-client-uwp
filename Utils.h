/***************************************************************************
 * Copyright (C) 2016 by Savoir-faire Linux                                *
 * Author: J�ger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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

#include <string>
#include <memory>
#include <WinNls.h>

namespace RingClientUWP
{
namespace Utils
{

std::string makeString(const std::wstring& wstr)
{
    auto wideData = wstr.c_str();
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideData,
        -1, nullptr, 0, NULL, NULL);

    std::unique_ptr<char[]> utf8;
    utf8.reset(new char[bufferSize]);

    if (WideCharToMultiByte(CP_UTF8, 0, wideData, -1,
        utf8.get(), bufferSize, NULL, NULL) == 0)
    {
        return std::string();
    }

    return std::string(utf8.get());
}

std::wstring makeWString(const std::string& str)
{
    auto utf8Data = str.c_str();
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, utf8Data, -1, nullptr, 0);

    std::unique_ptr<wchar_t[]> wide;
    wide.reset(new wchar_t[bufferSize]);

    if (MultiByteToWideChar(CP_UTF8, 0, utf8Data, -1,
        wide.get(), bufferSize) == 0)
    {
        return std::wstring();
    }

    return std::wstring(wide.get());;
}

std::string toString(Platform::String ^str)
{
    std::wstring wsstr(str->Data());
    return makeString(wsstr);
}

Platform::String^ toPlatformString(const std::string& str)
{
    std::wstring wsstr = makeWString(str);
    return ref new Platform::String(wsstr.c_str(), wsstr.length());
}

} /* end namespace Utils */
} /* end namespace RingClientUWP */