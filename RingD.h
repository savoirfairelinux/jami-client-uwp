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

using namespace concurrency;

namespace RingClientUWP
{
// TODO :: move the tasks in private, use enum for order
ref class Task
{
public:
    property String^ order;
};

ref class MessageText : public Task {

public:
    property String^ accountId;
    property String^ to;
    property String^ payload;
};

ref class File : public Task {

};

/* delegates */
delegate void IncomingCall(String^ accountId, String^ callId, String^ from);
delegate void StateChange(String^ callId, String^ state, int code);
delegate void MessageSent(String^ accountId, String^ to, String^ payload);

public ref class RingD sealed
{

public:
    /* functions */

    /* properties */
    static property RingD^ instance
    {
        RingD^ get()
        {
            static RingD^ instance_ = ref new RingD();
            return instance_;
        }
    }

    property bool daemonRunning
    {
        bool get()
        {
            return daemonRunning_;
        }
    }

internal:
    /* functions */
    void startDaemon();
    void dequeueTasks();

    void sendMessage(String^ accountId, String^ to, String^ payload);

    /* TODO : move members */
    bool hasConfig;
    std::string accountName;

    /* events */
    event IncomingCall^ incomingCall;
    event StateChange^ stateChange;
    event MessageSent^ messageSent;

private:
    RingD(); // singleton
    std::string localFolder_;
    bool daemonRunning_ = false;
    std::queue<Task^> tasksList_;
};
}