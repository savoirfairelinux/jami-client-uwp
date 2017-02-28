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
#include <type_traits>

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::System;
using namespace Windows::Globalization::DateTimeFormatting;

using VIS = Windows::UI::Xaml::Visibility;
static const uint64_t TICKS_PER_SECOND = 10000000;
static const uint64_t EPOCH_DIFFERENCE = 11644473600LL;

namespace RingClientUWP
{
namespace Utils
{

template<typename E>
constexpr inline typename std::enable_if<   std::is_enum<E>::value,
                                            typename std::underlying_type<E>::type
                                        >::type
toUnderlyingValue(E e) noexcept
{
    return static_cast<typename std::underlying_type<E>::type >( e );
}

template<typename E, typename T>
constexpr inline typename std::enable_if<   std::is_enum<E>::value &&
                                            std::is_integral<T>::value, E
                                        >::type
toEnum(T value) noexcept
{
    return static_cast<E>(value);
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
    if (toString(s).find("@") == std::string::npos)
        return s;

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

Windows::UI::Color
ColorFromString(String^ s)
{
    int a,r,g,b;
    if (sscanf_s(Utils::toString(s).c_str(), "#%2x%2x%2x%2x", &a, &r, &g, &b) == 4)
        return Windows::UI::ColorHelper::FromArgb(a, r, g, b);
    else
        return Windows::UI::ColorHelper::FromArgb(255, 0, 0, 0);
}

DateTime
epochToDateTime(std::time_t epochTime)
{
    Windows::Foundation::DateTime dateTime;
    dateTime.UniversalTime = (epochTime + EPOCH_DIFFERENCE) * TICKS_PER_SECOND;
    return dateTime;
}

DateTime
currentDateTime()
{
    Windows::Globalization::Calendar^ calendar = ref new Windows::Globalization::Calendar();
    return calendar->GetDateTime();
}

String^
dateTimeToString(DateTime dateTime)
{
    static auto timeFormatter = ref new DateTimeFormatter("day month year hour minute second");
    return timeFormatter->Format(dateTime);
}

std::time_t
dateTimeToEpoch(DateTime dateTime)
{
    return static_cast<std::time_t>(dateTime.UniversalTime / TICKS_PER_SECOND - EPOCH_DIFFERENCE);
}

} /*namespace Utils*/
} /*namespace RingClientUWP*/