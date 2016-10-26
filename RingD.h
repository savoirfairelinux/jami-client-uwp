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
// its ok to keep this enum here and to use it with the wizard, because in pch.h headers are a-z sorted,
// but it would be much more consistent to move this enum in globals.h when merged

public enum class StartingStatus { NORMAL, REGISTERING_ON_THIS_PC, REGISTERING_THIS_DEVICE };

/* delegate */
delegate void IncomingCall(String^ accountId, String^ callId, String^ from);
delegate void StateChange(String^ callId, CallStatus state, int code);
delegate void IncomingAccountMessage(String^ accountId, String^ from, String^ payload);
delegate void CallPlaced(String^ callId);
delegate void IncomingMessage(String^ callId, String^ payload);
delegate void DevicesListRefreshed(Vector<String^>^ devicesList);
delegate void ExportOnRingEnded(String^ accountId, String^ pin);
delegate void SummonWizard();
delegate void AccountUpdated(Account^ account);


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
    property StartingStatus _startingStatus;
    property String^ _pin; // you should RM ME
    property String^ _password; // refatco : think a little... RM ME

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
    void sendSIPTextMessage(String^ message);
    void createRINGAccount(String^ alias, String^ archivePassword, bool upnp);
    void createSIPAccount(String^ alias, String^ sipPassword, String^ sipHostname, String^ sipusername);
    void refuseIncommingCall(String^ call);
    void acceptIncommingCall(String^ call);
    void placeCall(Contact^ contact);
    /*void cancelOutGoingCall2(String^ callId);*/ // marche pas
    CallStatus translateCallStatus(String^ state);
    Vector<String^>^ translateKnownRingDevices(const std::map<std::string, std::string> devices);

    void hangUpCall2(String^ callId);
    void pauseCall(String ^ callId);
    void unPauseCall(String ^ callId);
    void askToRefreshKnownDevices(String^ accountId);
    void askToExportOnRing(String^ accountId, String^ password);
    void eraseCacheFolder();
    void updateAccount(String^ accountId);
    void deleteAccount(String^ accountId);

    /* TODO : move members */
    ///bool hasConfig; // replaced by startingStatus
    std::string accountName; // YOU SHOULD RM ME
    String ^ currentCallId; // to save ongoing call id during visibility change

    /* events */
    event IncomingCall^ incomingCall;
    event StateChange^ stateChange;
    event IncomingAccountMessage^ incomingAccountMessage;
    event IncomingMessage^ incomingMessage;
    event CallPlaced^ callPlaced;
    event DevicesListRefreshed^ devicesListRefreshed;
    event ExportOnRingEnded^ exportOnRingEnded;
    event SummonWizard^ summonWizard;
    event AccountUpdated^ accountUpdated;

private:
    /* sub classes */
    enum class Request {
        None,
        AddRingAccount,
        AddSIPAccount,
        RefuseIncommingCall,
        AcceptIncommingCall,
        CancelOutGoingCall,
        HangUpCall,
        PauseCall,
        UnPauseCall,
        RegisterDevice,
        GetKnownDevices,
        ExportOnRing,
        UpdateAccount,
        DeleteAccount
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
        Task(Request r, String^ p, String^ P) {
            request = r;
            _pin = p;
            _password = P;
        }
    public:
        property Request request;
        property String^ _alias;
        property String^ _callId;
        property String^ _pin;
        property String^ _password; // refacto : is it safe ? are tasks destroy quickly after been used ?
        property String^ _accountId;
        property bool _upnp;
        property String^ _sipPassword;
        property String^ _sipHostname;
        property String^ _sipUsername;
    };

    /* functions */
    RingD(); // singleton
    void dequeueTasks();
//    CallStatus translateCallStatus(String^ state);

    /* members */
    std::string localFolder_;
    bool daemonRunning_ = false;
    std::queue<Task^> tasksList_;
    StartingStatus startingStatus_ = StartingStatus::NORMAL;
    bool editModeOn_ = false;
};
}