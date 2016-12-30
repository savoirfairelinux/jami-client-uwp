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
#include <dring.h>

#include "Ringtone.h"
#include "Utils.h"

using namespace concurrency;

using namespace Windows::UI::Notifications;
using namespace Windows::Data::Xml::Dom;

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
delegate void IncomingVideoMuted(String^ callId, bool state);
delegate void RegisteredNameFound(LookupStatus status, const std::string& address, const std::string& name);
delegate void FinishCaptureDeviceEnumeration();
delegate void RegistrationStateErrorGeneric(const std::string& accountId);
delegate void RegistrationStateRegistered();
delegate void SetLoadingStatusText(String^ statusText, String^ color);
delegate void CallsListRecieved(const std::vector<std::string>& callsList);
delegate void AudioMuted(const std::string& callId, bool state);
delegate void VideoMuted(const std::string& callId, bool state);
delegate void NameRegistred(bool status);
delegate void VolatileDetailsChanged(const std::string& accountId, const std::map<std::string, std::string>& details);

using SharedCallback = std::shared_ptr<DRing::CallbackWrapperBase>;
using namespace std::placeholders;

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

    property bool daemonInitialized
    {
        bool get()
        {
            return daemonInitialized_;
        }
    }

    property bool daemonRunning
    {
        bool get()
        {
            return daemonRunning_;
        }
    }

    property bool isOnXBox
    {
        bool get()
        {
            auto dev = Utils::toString(Windows::System::Profile::AnalyticsInfo::VersionInfo->DeviceFamily);
            std::transform(dev.begin(), dev.end(), dev.begin(), ::tolower);
            return !dev.compare("windows.xbox");
        }
    }

    property bool isInBackground;
    property StartingStatus _startingStatus;

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
    std::string getLocalFolder();
    void registerCallbacks();
    void initDaemon(int flags);
    void startDaemon();
    void init();
    void deinit();
    void reloadAccountList();
    void sendAccountTextMessage(String^ message);
    void sendSIPTextMessage(String^ message);
    void sendSIPTextMessageVCF(std::string callID, std::map<std::string, std::string> message);
    void createRINGAccount(String^ alias, String^ archivePassword, bool upnp, String^ registeredName = "");
    void createSIPAccount(String^ alias, String^ sipPassword, String^ sipHostname, String^ sipusername);
    void refuseIncommingCall(String^ call);
    void acceptIncommingCall(String^ call);
    void placeCall(Contact^ contact);
    void pauseCall(const std::string& callId);
    void unPauseCall(const std::string& callId);
    /*void cancelOutGoingCall2(String^ callId);*/ // marche pas
    CallStatus translateCallStatus(String^ state);
    String^ getUserName();
    Vector<String^>^ translateKnownRingDevices(const std::map<std::string, std::string> devices);

    void hangUpCall2(String^ callId);
    void pauseCall(String ^ callId);
    void unPauseCall(String ^ callId);
    void askToRefreshKnownDevices(String^ accountId);
    void askToExportOnRing(String^ accountId, String^ password);
    void eraseCacheFolder();
    void updateAccount(String^ accountId);
    void deleteAccount(String^ accountId);
    void registerThisDevice(String^ pin, String^ archivePassword);
    void getCallsList();
    void killCall(String^ callId);
    void switchDebug();
    void muteVideo(String^ callId, bool muted);
    void muteAudio(const std::string& callId, bool muted);
    void lookUpName(String^ name);
    void registerName(String^ accountId, String^ password, String^ username);
    void registerName_new(const std::string& accountId, const std::string& password, const std::string& username);
    std::map<std::string, std::string> getVolatileAccountDetails(Account^ account);
    void lookUpAddress(String^ address);
    std::string registeredName(Account^ account);

    /* TODO : move members */
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
    event IncomingVideoMuted^ incomingVideoMuted;
    event RegisteredNameFound^ registeredNameFound;
    event FinishCaptureDeviceEnumeration^ finishCaptureDeviceEnumeration;
    event RegistrationStateErrorGeneric^ registrationStateErrorGeneric;
    event RegistrationStateRegistered^ registrationStateRegistered;
    event SetLoadingStatusText^ setLoadingStatusText;
    event CallsListRecieved^ callsListRecieved; // est implemente a la base pour regler le probleme du boutton d'appel qui est present lorsqu'un appel est en cours, mais il n'est pas utilise. Voir si ca peut servir a autre chose
    event AudioMuted^ audioMuted;
    event VideoMuted^ videoMuted;
    event NameRegistred^ nameRegistred;
    event VolatileDetailsChanged^ volatileDetailsChanged;

private:
    /* sub classes */
    enum class Request {
        None,
        AddRingAccount,
        AddSIPAccount,
        RefuseIncommingCall,
        AcceptIncommingCall,
        CancelOutGoingCall,
        PlaceCall,
        HangUpCall,
        PauseCall,
        UnPauseCall,
        RegisterDevice,
        GetKnownDevices,
        ExportOnRing,
        UpdateAccount,
        DeleteAccount,
        GetCallsList,
        KillCall,
        switchDebug,
        MuteVideo,
        MuteAudio,
        LookUpName,
        LookUpAddress,
        RegisterName
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
        property bool _muted;
        property String^ _registeredName; // public username
        property String^ _address; // ringId

    internal:
        std::string _accountId_new;
        std::string _password_new;
        std::string _publicUsername_new;
        std::string _callid_new;
        std::string _ringId_new;
        bool _audioMuted_new;
    };

    /* functions */
    RingD(); // singleton
    void dequeueTasks();
//    CallStatus translateCallStatus(String^ state);

    /* members */
    Windows::UI::Core::CoreDispatcher^ dispatcher;

    std::string localFolder_;
    bool daemonInitialized_ = false;
    bool daemonRunning_ = false;
    std::queue<Task^> tasksList_;
    StartingStatus startingStatus_ = StartingStatus::NORMAL;
    bool editModeOn_ = false;
    bool debugModeOn_ = true;
    Ringtone^ ringtone_;

    std::map<std::string, SharedCallback> callHandlers;
    std::map<std::string, SharedCallback> getAppPathHandler;
    std::map<std::string, SharedCallback> getAppUserNameHandler;
    std::map<std::string, SharedCallback> incomingVideoHandlers;
    std::map<std::string, SharedCallback> outgoingVideoHandlers;
    std::map<std::string, SharedCallback> nameRegistrationHandlers;
};
}
