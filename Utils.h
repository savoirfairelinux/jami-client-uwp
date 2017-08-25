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

#include "StringUtils.h"

#include <random>
#include <type_traits>

using namespace Platform;
using namespace Platform::Collections;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace Windows::Security::Cryptography;
using namespace Windows::Security::Cryptography::Core;

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

String^
getDetailsStringValue(Map<String^, String^>^ details, const char* key)
{
    auto Key = toPlatformString(key);
    if (details->HasKey(Key))
        return details->Lookup(Key);
    return nullptr;
}

bool
getDetailsBoolValue(Map<String^, String^>^ details, const char* key)
{
    auto Key = toPlatformString(key);
    if (details->HasKey(Key))
        return details->Lookup(Key) == "true";
    return false;
}

String^
computeMD5(String^ strMsg)
{
    String^ strAlgName = HashAlgorithmNames::Md5;
    IBuffer^ buffUtf8Msg = CryptographicBuffer::ConvertStringToBinary(strMsg, BinaryStringEncoding::Utf8);
    HashAlgorithmProvider^ algProv = HashAlgorithmProvider::OpenAlgorithm(strAlgName);
    IBuffer^ buffHash = algProv->HashData(buffUtf8Msg);
    String^ hex = CryptographicBuffer::EncodeToHexString(buffHash);
    return hex;
}

} /*namespace Utils*/

} /*namespace RingClientUWP*/