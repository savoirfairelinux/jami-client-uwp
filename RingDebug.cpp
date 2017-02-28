/***************************************************************************
 * Copyright (C) 2016 by Savoir-faire Linux                                *
 * Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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

/* client */
#include "pch.h"

using namespace RingClientUWP;

using namespace Platform;
using namespace Windows::UI::Core;
using namespace Windows::Storage;

using namespace std::chrono;

std::string
getDebugHeader(std::string file, int line)
{
    auto tid = std::this_thread::get_id();

    seconds s = duration_cast< seconds >(
        system_clock::now().time_since_epoch()
    );
    milliseconds ms = duration_cast< milliseconds >(
        system_clock::now().time_since_epoch()
    );

    static uint64_t secs = s.count();
    static uint64_t millis = ms.count() - (secs * 1000);

    std::ostringstream out;
    const auto prev_fill = out.fill();
    out << '[' << secs
        << '.' << std::right << std::setw(3) << std::setfill('0') << millis << std::left
        << '|' << std::right << std::setw(5) << std::setfill(' ') << tid << std::left;
    out.fill(prev_fill);
    out << "|" << std::setw(32) << (file + ':' + Utils::toString((line.ToString())));
    out << "] ";

    return out.str();
}

void
RingDebug::print(const  std::string& message, const Type& type,
                        std::string file, int line)
{
    Utils::runOnWorkerThread([this, message, type, file, line]()
    {
        std::string _message;

        if (type != Type::DMN)
            _message = getDebugHeader(file, line) + message;
        else
            _message = message;

        std::wstring wString = std::wstring(_message.begin(), _message.end());

        /* set message type. */
        wString = (type>Type::WNG) ? (L"(EE) ") : ((type>Type::MSG) ? (L"(WW) ") : (L"")) + wString;

        String^ msg;
        msg = ref new String(wString.c_str(), wString.length());

#if UWP_DBG_VS
        /* screen it into VS debug console */
        OutputDebugString((wString + L"\n").c_str());
#endif

#if UWP_DBG_CONSOLE
        /* fire the event. */
        messageToScreen(msg);
#endif

#if UWP_DBG_FILE
        /* output to file */
        printToLogFile(Utils::toString(msg));
#endif
    }, WorkItemPriority::Normal);
}

void
RingDebug::print(String^ message, const Type& type,
                        std::string file, int line)
{
    Utils::runOnWorkerThread([this, message, type, file, line]()
    {
        /* add header */
        auto messageTimestamped = Utils::toPlatformString(getDebugHeader(file, line)) + message;

        /* set message type. */
        messageTimestamped = (type>Type::WNG) ? ("(EE) ") : ((type>Type::MSG) ? ("(WW) ") : ("")) + messageTimestamped;

#if UWP_DBG_VS
        /* screen it into VS debug console */
        std::wstringstream wStringstream;
        wStringstream << messageTimestamped->Data() << "\n";
        OutputDebugString(wStringstream.str().c_str());
#endif

#if UWP_DBG_CONSOLE
        /* fire the event. */
        messageToScreen(messageTimestamped);
#endif

#if UWP_DBG_FILE
        /* output to file */
        printToLogFile(Utils::toString(messageTimestamped));
#endif
    }, WorkItemPriority::Normal);
}

void
RingDebug::print(Exception^ e, std::string file, int line)
{
    Utils::runOnWorkerThread([this, e, file, line]()
    {
        /* add header */
        auto message = Utils::toPlatformString(getDebugHeader(file, line)) + "Exception : 0x" + e->HResult.ToString() + ": " + e->Message;

#if UWP_DBG_VS
        /* screen it into VS debug console */
        std::wstringstream wStringstream;
        wStringstream << message->Data() << "\n";
        OutputDebugString(wStringstream.str().c_str());
#endif

#if UWP_DBG_CONSOLE
        /* fire the event. */
        messageToScreen(message);
#endif

#if UWP_DBG_FILE
        /* output to file */
        printToLogFile(Utils::toString(message));
#endif
    }, WorkItemPriority::Normal);
}

RingDebug::RingDebug()
{
    /* clean the log file */
    std::ofstream ofs;
    ofs.open (RingD::instance->getLocalFolder() + "debug.log", std::ofstream::out | std::ofstream::trunc);
    ofs.close();
}

void
RingDebug::printToLogFile(std::string msg)
{
    {
        std::lock_guard<std::mutex> l(debugMutex_);
        std::ofstream ofs;
        ofs.open(RingD::instance->getLocalFolder() + "debug.log", std::ofstream::out | std::ofstream::app);
        ofs << msg << "\n";
        ofs.close();
    }
}
