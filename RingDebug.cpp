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

void
RingDebug::print(const std::string& message,
                 const Type& type)
{
    /* get the current time */
    std::time_t currentTime = std::time(nullptr);
    char timeBuffer[64];
    ctime_s(timeBuffer, sizeof timeBuffer, &currentTime);

    /* timestamp */
    auto messageTimestamped = timeBuffer + message;
    std::wstring wString = std::wstring(message.begin(), message.end());

    /* set message type. */
    switch (type) {
    case Type::ERR:
        wString = L"(EE) " + wString;
        break;
    case Type::WNG:
        wString = L"(WW) " + wString;
        break;
        /*case Type::message:*/
    }

    /* screen it into VS debug console */
    OutputDebugString((wString + L"\n").c_str());

    /* fire the event. */
    auto line = ref new String(wString.c_str(), wString.length());
    messageToScreen(line);
    FileIO::AppendTextAsync(_logFile, line + "\n");
}

void RingClientUWP::RingDebug::WriteLine(String^ str)
{
    /* save in file */
    //FileIO::AppendTextAsync(_videoFile, str + "\n");

    /* screen in visual studio console */
    std::wstringstream wStringstream;
    wStringstream << str->Data() << "\n";
    OutputDebugString(wStringstream.str().c_str());
}

RingClientUWP::RingDebug::RingDebug()
{
    StorageFolder^ storageFolder = ApplicationData::Current->LocalFolder;

    StorageFile^ logFile;

    task<StorageFile^>(storageFolder->CreateFileAsync("debug.log", CreationCollisionOption::ReplaceExisting)).then([this](StorageFile^ file)
    {
        this->_logFile = file;
    });

    task<StorageFile^>(storageFolder->CreateFileAsync("video.log", CreationCollisionOption::ReplaceExisting)).then([this](StorageFile^ file)
    {
        this->_videoFile = file;
    });

}

