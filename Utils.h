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
#include <functional>

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::System;
using namespace Windows::Globalization::DateTimeFormatting;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Media::Imaging;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::Networking::Connectivity;
using namespace Windows::System::Threading;

using VIS = Windows::UI::Xaml::Visibility;
static const uint64_t TICKS_PER_SECOND = 10000000;
static const uint64_t EPOCH_DIFFERENCE = 11644473600LL;

namespace RingClientUWP
{
namespace Utils
{

namespace detail
{

#include <stdint.h>
#include <stdlib.h>

/* Mainly based on the following stackoverflow question:
* http://stackoverflow.com/questions/342409/how-do-i-base64-encode-decode-in-c
*/
static const char encoding_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K',
    'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V',
    'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r',
    's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2',
    '3', '4', '5', '6', '7', '8', '9', '+', '/'
};

static const size_t mod_table[] = { 0, 2, 1 };

char *base64_encode(const uint8_t *input, size_t input_length,
    char *output, size_t *output_length)
{
    size_t i, j;
    size_t out_sz = *output_length;
    *output_length = 4 * ((input_length + 2) / 3);
    if (out_sz < *output_length || output == NULL)
        return NULL;

    for (i = 0, j = 0; i < input_length; ) {
        uint8_t octet_a = i < input_length ? input[i++] : 0;
        uint8_t octet_b = i < input_length ? input[i++] : 0;
        uint8_t octet_c = i < input_length ? input[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        output[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        output[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        output[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        output[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (i = 0; i < mod_table[input_length % 3]; i++)
        output[*output_length - 1 - i] = '=';

    return output;
}

uint8_t *base64_decode(const char *input, size_t input_length,
    uint8_t *output, size_t *output_length)
{
    size_t i, j;
    uint8_t decoding_table[256];

    uint8_t c;
    for (c = 0; c < 64; c++)
        decoding_table[static_cast<int>(encoding_table[c])] = c;

    if (input_length % 4 != 0)
        return NULL;

    size_t out_sz = *output_length;
    *output_length = input_length / 4 * 3;
    if (input[input_length - 1] == '=')
        (*output_length)--;
    if (input[input_length - 2] == '=')
        (*output_length)--;

    if (out_sz < *output_length || output == NULL)
        return NULL;

    for (i = 0, j = 0; i < input_length;) {
        uint8_t sextet_a = input[i] == '=' ? 0 & i++
            : decoding_table[static_cast<int>(input[i++])];
        uint8_t sextet_b = input[i] == '=' ? 0 & i++
            : decoding_table[static_cast<int>(input[i++])];
        uint8_t sextet_c = input[i] == '=' ? 0 & i++
            : decoding_table[static_cast<int>(input[i++])];
        uint8_t sextet_d = input[i] == '=' ? 0 & i++
            : decoding_table[static_cast<int>(input[i++])];

        uint32_t triple = (sextet_a << 3 * 6) +
            (sextet_b << 2 * 6) +
            (sextet_c << 1 * 6) +
            (sextet_d << 0 * 6);

        if (j < *output_length)
            output[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < *output_length)
            output[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < *output_length)
            output[j++] = (triple >> 0 * 8) & 0xFF;
    }

    return output;
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

    return std::wstring(wide.get());
}

} /*namespace detail*/

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

inline std::string
toString(Platform::String ^str)
{
    std::wstring wsstr(str->Data());
    return detail::makeString(wsstr);
}

inline Platform::String^
toPlatformString(const std::string& str)
{
    std::wstring wsstr = detail::makeWString(str);
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
genID(uint64_t lower, uint64_t upper)
{
    std::random_device r;
    std::mt19937 gen(r());
    std::uniform_int_distribution<uint64_t> idgen {lower, upper};

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

bool
hasInternet()
{
    auto connectionProfile = NetworkInformation::GetInternetConnectionProfile();
    return (connectionProfile != nullptr &&
        connectionProfile->GetNetworkConnectivityLevel() == NetworkConnectivityLevel::InternetAccess);
}

std::string
getHostName()
{
    auto hostNames = NetworkInformation::GetHostNames();
    auto hostName = hostNames != nullptr ? toString(hostNames->GetAt(0)->DisplayName) : "";
    return hostName;
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

template <typename... Args>
void
runOnUIThreadAfter(int delayInMilliSeconds, std::function<void()> const& f)
{
    // duration is measured in 100-nanosecond units
    TimeSpan delay;
    delay.Duration = 10000 * delayInMilliSeconds;
    ThreadPoolTimer^ delayTimer = ThreadPoolTimer::CreateTimer(
        ref new TimerElapsedHandler([=](ThreadPoolTimer^ source)
    {
        CoreApplication::MainView->CoreWindow->Dispatcher->RunAsync(CoreDispatcherPriority::High,
            ref new DispatchedHandler([=]()
        {
            // invoke func
            f();
        }));
    }), delay);
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

std::time_t
currentTimestamp()
{
    return std::time(nullptr);
}

String^
dateTimeToString(DateTime dateTime, String^ format)
{
    auto timeFormatter = ref new DateTimeFormatter(format);
    return timeFormatter->Format(dateTime);
}

std::time_t
dateTimeToEpoch(DateTime dateTime)
{
    return static_cast<std::time_t>(dateTime.UniversalTime / TICKS_PER_SECOND - EPOCH_DIFFERENCE);
}

namespace xaml
{

Windows::UI::Xaml::FrameworkElement^
FindVisualChildByName(DependencyObject^ obj, String^ name)
{
    FrameworkElement^ ret;
    int numChildren = VisualTreeHelper::GetChildrenCount(obj);
    for (int i = 0; i < numChildren; i++)
    {
        auto objChild = VisualTreeHelper::GetChild(obj, i);
        auto child = safe_cast<FrameworkElement^>(objChild);
        if (child != nullptr && child->Name == name)
        {
            return child;
        }

        ret = FindVisualChildByName(objChild, name);
        if (ret != nullptr)
            break;
    }
    return ret;
}

} /*namespace xaml*/

namespace base64
{

std::string
encode(const std::vector<uint8_t>::const_iterator begin,
    const std::vector<uint8_t>::const_iterator end)
{
    size_t output_length = 4 * ((std::distance(begin, end) + 2) / 3);
    std::string out;
    out.resize(output_length);
    detail::base64_encode(&(*begin), std::distance(begin, end),
        &(*out.begin()), &output_length);
    out.resize(output_length);
    return out;
}

std::string
encode(const std::vector<uint8_t>& dat)
{
    return encode(dat.cbegin(), dat.cend());
}

std::vector<uint8_t>
decode(const std::string& str)
{
    size_t output_length = str.length() / 4 * 3 + 2;
    std::vector<uint8_t> output;
    output.resize(output_length);
    detail::base64_decode(str.data(), str.size(), output.data(), &output_length);
    output.resize(output_length);
    return output;
}

} /*namespace base64*/

} /*namespace Utils*/
} /*namespace RingClientUWP*/