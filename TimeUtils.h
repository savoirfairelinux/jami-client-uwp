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

#include <ctime>
#include <cstdint>

using namespace Platform;
using namespace Windows::System;
using namespace Windows::Globalization::DateTimeFormatting;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;

namespace RingClientUWP
{
namespace Utils
{

namespace Time
{

static const uint64_t TICKS_PER_SECOND = 10000000;
static const uint64_t EPOCH_DIFFERENCE = 11644473600LL;

String^ computedShortDateTimeString;
Windows::Globalization::Calendar^ calendar = ref new Windows::Globalization::Calendar();
DateTimeFormatter^ shortdateTimeFormatter = ref new DateTimeFormatter("shortdate");
DateTimeFormatter^ dayofweekTimeFormatter = ref new DateTimeFormatter("dayofweek");
DateTimeFormatter^ hourminuteTimeFormatter = ref new DateTimeFormatter("hour minute");

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
    if (format == "shortdate")
        return shortdateTimeFormatter->Format(dateTime);
    else if (format == "dayofweek")
        return dayofweekTimeFormatter->Format(dateTime);
    else if (format == "hour minute")
        return hourminuteTimeFormatter->Format(dateTime);
    return nullptr;
}

std::time_t
dateTimeToEpoch(DateTime dateTime)
{
    return static_cast<std::time_t>(dateTime.UniversalTime / TICKS_PER_SECOND - EPOCH_DIFFERENCE);
}

} /*namespace Time*/

} /*namespace Utils*/

} /*namespace RingClientUWP*/
