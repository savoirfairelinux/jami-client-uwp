/***************************************************************************
* Copyright (C) 2016 by Savoir-faire Linux                                *
* Author: Jäger Nicolas <nicolas.jager@savoirfairelinux.com>              *
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

#define UWP_DBG_CONSOLE     0
#define UWP_DBG_VS          1
#define UWP_DBG_FILE        1

#define UWP_DBG_CLIENT      1
#define UWP_DBG_IO          0
#define UWP_DBG_DAEMON      1

using namespace Windows::Storage;

namespace RingClientUWP
{

/* forward declaration */
ref class RingDebug;

/* delegate */
delegate void debugMessageToScreen(Platform::String^ message);

/* this is how to implement a singleton class*/
public ref class RingDebug sealed
{
public:
    /* singleton */
    static property RingDebug^ instance
    {
        RingDebug^ get()
        {
            static RingDebug^ instance_ = ref new RingDebug();
            return instance_;
        }
    }

    /* properties */

    /* functions */
internal:
    enum class Type { DMN, MSG, WNG, ERR };
    void print(const std::string& message, const Type& type,
               std::string file, int line);
    void print(String^ message, const Type& type,
               std::string file, int line);
    void print(Exception^ e, std::string file, int line);

    void printToLogFile(std::string msg);

    /* event */
    event debugMessageToScreen^ messageToScreen;

private:
    std::mutex debugMutex_;
    RingDebug(); // singleton
};

void
WriteLine(String^ str)
{
    std::wstringstream wStringstream;
    wStringstream << str->Data() << "\n";
    OutputDebugString(wStringstream.str().c_str());
}

__declspec(deprecated)
void
WriteException(Exception^ ex)
{
    std::wstringstream wStringstream;
    wStringstream << "0x" << ex->HResult << ": " << ex->Message->Data();
    OutputDebugString(wStringstream.str().c_str());
}

#if UWP_DBG_DAEMON
#define DMSG_(str)  RingDebug::instance->print(str, RingDebug::Type::DMN, __FILE__, __LINE__)
#else
#define DMSG_(str)
#endif

#if UWP_DBG_CLIENT
#define MSG_(str)   RingDebug::instance->print(str, RingDebug::Type::MSG, __FILE__, __LINE__)
#define WNG_(str)   RingDebug::instance->print(str, RingDebug::Type::WNG, __FILE__, __LINE__)
#define ERR_(str)   RingDebug::instance->print(str, RingDebug::Type::ERR, __FILE__, __LINE__)
#define EXC_(e)     RingDebug::instance->print(e, __FILE__, __LINE__)

#if UWP_DBG_IO
#define IOMSG_(str) MSG_(str)
#else
#define IOMSG_(str)
#endif
#else
#define MSG_(str)
#define WNG_(str)
#define ERR_(str)
#define EXC_(e)
#define IOMSG_(str)
#endif

}
