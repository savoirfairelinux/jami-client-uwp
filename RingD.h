/**************************************************************************
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

using namespace concurrency;

namespace RingClientUWP
{

/* delegate */
delegate void IncomingCall(String^ accountId, String^ callId, String^ from);
delegate void StateChange(String^ callId, CallStatus state, int code);
delegate void IncomingAccountMessage(String^ accountId, String^ from, String^ payload);
delegate void CallPlaced(String^ callId);


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

    void cancelOutGoingCall2(String^ callId); // marche

internal: // why this property has to be internal and not public ?
    property Vector<String^>^ _callIdsList
    {
        Vector<String^>^ get()
        {
            return callIdsList_;
        }
    }


internal:
    /* functions */
    void startDaemon();
    void reloadAccountList();
    void sendAccountTextMessage(String^ message);
    void createRINGAccount(String^ alias);
    void createSIPAccount(String^ alias);
    void refuseIncommingCall(String^ call);
    void acceptIncommingCall(String^ call);
    void placeCall(Contact^ contact);
    /*void cancelOutGoingCall2(String^ callId);*/ // marche pas
    CallStatus getCallStatus(String^ state);

    void hangUpCall2(String^ callId);

    /* TODO : move members */
    bool hasConfig;
    std::string accountName;

    /* events */
    event IncomingCall^ incomingCall;
    event StateChange^ stateChange;
    event IncomingAccountMessage^ incomingAccountMessage;
    event CallPlaced^ callPlaced;

private:
    /* sub classes */
    enum class Request {
        None,
        AddRingAccount,
        AddSIPAccount,
        RefuseIncommingCall,
        AcceptIncommingCall,
        CancelOutGoingCall,
        HangUpCall
    };


    Vector<String^>^ callIdsList_;

    ref class Task
    {
    internal:
        Task(Request r) {
            request = r;
        }
        Task(Request r, String^ c) {
            request = r;
            _callId = c;
        }
    public:
        property Request request;
        property String^ _callId;
    };

    /* functions */
    RingD(); // singleton
    void dequeueTasks();
//    CallStatus getCallStatus(String^ state);

    /* members */
    std::string localFolder_;
    bool daemonRunning_ = false;
    std::queue<Task^> tasksList_;
};
}