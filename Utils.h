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
#include <pch.h>

#include <random>

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::System;

typedef Windows::UI::Xaml::Visibility VIS;

namespace RingClientUWP
{
namespace Utils
{
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

inline std::string
makeString(const std::wstring& wstr)
{
    auto wideData = wstr.c_str();
    int bufferSize = WideCharToMultiByte(CP_UTF8, 0, wideData, -1, nullptr, 0, NULL, NULL);

    std::unique_ptr<char[]> utf8;
    utf8.reset(new char[bufferSize]);

    if (WideCharToMultiByte(CP_UTF8, 0, wideData, -1, utf8.get(), bufferSize, NULL, NULL) == 0) {
        return std::string();
    }

    return std::string(utf8.get());
}

inline std::wstring
makeWString(const std::string& str)
{
    auto utf8Data = str.c_str();
    int bufferSize = MultiByteToWideChar(CP_UTF8, 0, utf8Data, -1, nullptr, 0);

    std::unique_ptr<wchar_t[]> wide;
    wide.reset(new wchar_t[bufferSize]);

    if (MultiByteToWideChar(CP_UTF8, 0, utf8Data, -1, wide.get(), bufferSize) == 0) {
        return std::wstring();
    }

    return std::wstring(wide.get());;
}

inline std::string
toString(Platform::String ^str)
{
    std::wstring wsstr(str->Data());
    return makeString(wsstr);
}

inline Platform::String^
toPlatformString(const std::string& str)
{
    std::wstring wsstr = makeWString(str);
    return ref new Platform::String(wsstr.c_str(), wsstr.length());
}

Platform::String^
Trim(Platform::String^ s)
{
    const WCHAR* first = s->Begin();
    const WCHAR* last = s->End();

    while (first != last && iswspace(*first))
        ++first;

    while (first != last && iswspace(last[-1]))
        --last;

    return ref new Platform::String(first, static_cast<unsigned int>(last - first));
}

/* fix some issue in the daemon --> <...@...> */
Platform::String^
TrimRingId(Platform::String^ s)
{
    const WCHAR* first = s->Begin();
    const WCHAR* last = s->End();

    while (first != last && *first != '<')
        ++first;

    while (first != last && last[-1] != '@')
        --last;

    first++;
    last--;

    return ref new Platform::String(first, static_cast<unsigned int>(last - first));
}

/* fix some issue in the daemon -->  remove "@..." */
Platform::String^

TrimRingId2(Platform::String^ s)
{
    const WCHAR* first = s->Begin();
    const WCHAR* last = s->End();

    while (first != last && last[-1] != '@')
        --last;

    last--;

    return ref new Platform::String(first, static_cast<unsigned int>(last - first));
}

Platform::String^
TrimFrom(Platform::String^ s)
{
    const WCHAR* first = s->Begin();
    const WCHAR* last = s->End();

    while (first != last && *first != ':')
        ++first;

    while (first != last && last[-1] != '>')
        --last;

    first++;
    last--;

    return ref new Platform::String(first, static_cast<unsigned int>(last - first));
}

Platform::String^
TrimCmd(Platform::String^ s)
{
    const WCHAR* first = s->Begin();
    const WCHAR* last = s->End();

    while (first != last && last[0] != '\ ')
        --last;

        //last--;

    return ref new Platform::String(first, sizeof(last));
}

Platform::String^
TrimParameter(Platform::String^ s)
{
    const WCHAR* first = s->Begin();
    const WCHAR* last = s->End();

    while (first != last && *first != '[')
        ++first;

    while (first != last && last[-1] != ']')
        --last;

    first++;
    last--;

    if (static_cast<unsigned int>(last - first) > 0)
        return ref new Platform::String(first, static_cast<unsigned int>(last - first));
    else
        return "";
}

Platform::String^
GetNewGUID()
{
    GUID result;
    HRESULT hr = CoCreateGuid(&result);

    if (SUCCEEDED(hr)) {
        Guid guid(result);
        return guid.ToString();
    }

    throw Exception::CreateException(hr);
}

std::string
getStringFromFile(const std::string& filename)
{
    std::ifstream file(filename, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(file)),
        (std::istreambuf_iterator<char>()));
}

inline Map<String^, String^>^
convertMap(const std::map<std::string, std::string>& m)
{
    auto temp = ref new Map<String^, String^>;
    for (const auto& pair : m) {
        temp->Insert(
            Utils::toPlatformString(pair.first),
            Utils::toPlatformString(pair.second)
        );
    }
    return temp;
}

inline std::map<std::string, std::string>
convertMap(Map<String^, String^>^ m)
{
    std::map<std::string, std::string> temp;
    for (const auto& pair : m) {
        temp.insert(
            std::make_pair(
                Utils::toString(pair->Key),
                Utils::toString(pair->Value)));
    }
    return temp;
}

template<class T>
bool
findIn(std::vector<T> vec, T val)
{
    if (std::find(vec.begin(), vec.end(), val) == vec.end())
        return true;
    return false;
}

std::string
genID(long long lower, long long upper)
{
    std::random_device r;
    std::mt19937 gen(r());
    std::uniform_int_distribution<long long> idgen {lower, upper};

    uint16_t digits = 0;
    if (upper < 0LL)
        digits = 1;
    while (upper) {
        upper /= 10LL;
        digits++;
    }

    std::ostringstream o;
    o.fill('0');
    o.width(digits);
    o << idgen(gen);

    return o.str();
}


} /*namespace Utils*/
} /*namespace RingClientUWP*/