/**************************************************************************
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
#include <dring.h>

#include "Ringtone.h"
#include "Utils.h"

using namespace concurrency;

using namespace Windows::UI::Notifications;
using namespace Windows::Data::Xml::Dom;
using namespace Windows::UI::ViewManagement;

namespace RingClientUWP
{

public enum class StartingStatus { NORMAL, REGISTERING_ON_THIS_PC, REGISTERING_THIS_DEVICE };

/* delegates */
delegate void IncomingCall(String^ accountId, String^ callId, String^ from);
delegate void StateChange(String^ callId, CallStatus state, int code);
delegate void IncomingAccountMessage(String^ accountId, String^ from, String^ payload);
delegate void CallPlaced(String^ callId);
delegate void IncomingMessage(String^ callId, String^ payload);
delegate void DevicesListRefreshed(Map<String^, String^>^ deviceMap);
delegate void ExportOnRingEnded(String^ accountId, String^ pin);
delegate void SummonWizard();
delegate void AccountUpdated(Account^ account);
delegate void IncomingVideoMuted(String^ callId, bool state);
delegate void RegisteredNameFound(LookupStatus status, const std::string& accountId, const std::string& address, const std::string& name);
delegate void FinishCaptureDeviceEnumeration();
delegate void RegistrationStateErrorGeneric(const std::string& accountId);
delegate void RegistrationStateRegistered(const std::string& accountId);
delegate void RegistrationStateUnregistered(const std::string& accountId);
delegate void RegistrationStateTrying(const std::string& accountId);
delegate void SetOverlayStatusText(String^ statusText, String^ color);
delegate void CallsListRecieved(const std::vector<std::string>& callsList);
delegate void AudioMuted(const std::string& callId, bool state);
delegate void VideoMuted(const std::string& callId, bool state);
delegate void FullScreenToggled(bool state);
delegate void WindowResized(float width, float height);
delegate void NetworkChanged();
delegate void MessageDataLoaded();
delegate void UpdateSmartInfo(const std::map<std::string, std::string>& info);
delegate void MessageStatusUpdated(String^ messageId, int status);
delegate void VolatileDetailsChanged(const std::string& accountId, const std::map<std::string, std::string>& details);
delegate void NewBuddyNotification(const std::string& accountId, const std::string& uri, int status);
delegate void VCardUpdated(Contact^ owner);
delegate void ShareRequested();
delegate void NameRegistered(bool status, String^ accountId);

using SharedCallback = std::shared_ptr<DRing::CallbackWrapperBase>;
using namespace std::placeholders;

public ref class RingD sealed
{
public:
    /* properties */
    static property RingD^ instance {
        RingD^ get() {
            static RingD^ instance_ = ref new RingD();
            return instance_;
        }
    }

    property bool daemonInitialized {
        bool get() {
            return daemonInitialized_;
        }
    }

    property bool daemonRunning {
        bool get() {
            return daemonRunning_;
        }
    }

    property bool _hasInternet {
        bool get() {
            return hasInternet_;
        }
    }

    property bool isInWizard;

    property bool isFullScreen {
        bool get() {
            return ApplicationView::GetForCurrentView()->IsFullScreenMode;
        }
    }

    property bool isOnXBox {
        bool get() {
            auto dev = Utils::toString(Windows::System::Profile::AnalyticsInfo::VersionInfo->DeviceFamily);
            std::transform(dev.begin(), dev.end(), dev.begin(), ::tolower);
            return !dev.compare("windows.xbox");
        }
    }

    property bool isInBackground {
        bool get() {
            return isInBackground_;
        }
        void set(bool value) {
            isInBackground_ = value;
            connectivityChanged();
        }
    }

    property bool isInvisible {
        bool get() {
            return isInvisible_;
        }
        void set(bool value) {
            isInvisible_ = value;
        }
    }

    property Ringtone^ ringtone {
        Ringtone^ get() {
            return ringtone_;
        }
    }

    property MainPage^ mainPage {
        MainPage^ get() {
            auto frame = dynamic_cast<Windows::UI::Xaml::Controls::Frame^>(Window::Current->Content);
            return dynamic_cast<RingClientUWP::MainPage^>(frame->Content);
        }
    }

    property StartingStatus _startingStatus;

    property bool isCtrlPressed;
    property bool isShiftPressed;

    /* functions */
    void cancelOutGoingCall2(String^ callId); // marche
    void raiseWindowResized(float width, float height);
    void raiseShareRequested();

internal: // why this property has to be internal and not public ?
    property Vector<String^>^ _callIdsList
    {
        Vector<String^>^ get()
        {
            return callIdsList_;
        }
    }

    std::map<String^, std::function<void(String^ username)>> unpoppedToasts;

    using AccountDetails = std::map<std::string, std::string>;
    using AccountDetailsBlob = std::map<std::string, AccountDetails>;
    /* functions */
    std::string getLocalFolder();
    void registerCallbacks();
    void initDaemon(int flags);
    void startDaemon();
    void init();
    void deinit();
    AccountDetailsBlob getAllAccountDetails();
    void parseAccountDetails(const AccountDetailsBlob& allAccountDetails);
    void subscribeBuddies();
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
    CallStatus translateCallStatus(String^ state);
    String^ getUserName();
    void startSmartInfo(int refresh);
    void stopSmartInfo();
    void handleIncomingMessage( const std::string& callId,
                                const std::string& accountId,
                                const std::string& from,
                                const std::map<std::string, std::string>& payloads);

    void toggleFullScreen();
    void setWindowedMode();
    void setFullScreenMode();
    void connectivityChanged();
    void onStateChange(Platform::String ^callId, RingClientUWP::CallStatus state, int code);
    void hangUpCall2(String^ callId);
    void pauseCall(String ^ callId);
    void unPauseCall(String ^ callId);
    void getKnownDevices(String^ accountId);
    void ExportOnRing(String^ accountId, String^ password);
    void updateAccount(String^ accountId);
    void deleteAccount(String^ accountId);
    void registerThisDevice(String^ pin, String^ archivePassword);
    void muteVideo(String^ callId, bool muted);
    void muteAudio(const std::string& callId, bool muted);
    void subscribeBuddy(const std::string& accountId, const std::string& uri, bool flag);
    void registerName(String^ accountId, String^ password, String^ username);
    std::map<std::string, std::string> getVolatileAccountDetails(Account^ account);
    void lookUpName(const std::string& accountId, String^ name);
    void lookUpAddress(const std::string& accountId, String^ address);
    std::string registeredName(Account^ account);
    void removeContact(const std::string & accountId, const std::string& uri);
    void sendContactRequest(const std::string& accountId, const std::string& uri, const std::string& payload);
    void raiseMessageDataLoaded();
    void raiseVCardUpdated(Contact^ contact);
    void revokeDevice(const std::string& accountId, const std::string& password, const std::string& deviceId);
    void showLoadingOverlay(String^ text, String^ color);
    void hideLoadingOverlay(String^ text, String^ color, int delayInMilliseconds = 2000);
    void OnaccountAdded(const std::string& accountId);
    void OnaccountUpdated();
    void OnaccountDeleted();

    void ShowCallToast(bool background, String^ callId, String^ name = nullptr);
    void ShowIMToast(bool background, String^ from, String^ payload, String^ name = nullptr);
    void HideToast(ToastNotification^ toast);

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
    event RegistrationStateUnregistered^ registrationStateUnregistered;
    event RegistrationStateTrying^ registrationStateTrying;
    event SetOverlayStatusText^ setOverlayStatusText;
    event CallsListRecieved^ callsListRecieved; // est implemente a la base pour regler le probleme du boutton d'appel qui est present lorsqu'un appel est en cours, mais il n'est pas utilise. Voir si ca peut servir a autre chose
    event AudioMuted^ audioMuted;
    event VideoMuted^ videoMuted;
    event FullScreenToggled^ fullScreenToggled;
    event WindowResized^ windowResized;
    event NetworkChanged^ networkChanged;
    event MessageDataLoaded^ messageDataLoaded;
    event UpdateSmartInfo^ updateSmartInfo;
    event MessageStatusUpdated^ messageStatusUpdated;
    event VolatileDetailsChanged^ volatileDetailsChanged;
    event NewBuddyNotification^ newBuddyNotification;
    event VCardUpdated^ vCardUpdated;
    event ShareRequested^ shareRequested;
    event NameRegistered^ nameRegistered;

private:
    Vector<String^>^ callIdsList_;

    /* functions */
    RingD(); // singleton

    void InternetConnectionChanged(Platform::Object^ sender);
    //CallStatus translateCallStatus(String^ state);

    /* members */
    Windows::UI::Core::CoreDispatcher^ dispatcher;

    bool hasInternet_;
    bool isAddingDevice     = false;
    bool isAddingAccount    = false;
    bool shouldRegister     = false;
    String^ nameToRegister  = "";
    bool isUpdatingAccount  = false;
    bool isDeletingAccount  = false;
    bool isInBackground_    = false;
    bool isInvisible_       = false;
    bool daemonInitialized_ = false;
    bool daemonRunning_     = false;
    bool debugModeOn_       = true;
    bool callToastPopped_   = false;

    std::string localFolder_;
    Utils::task_queue tasks_;
    StartingStatus startingStatus_ = StartingStatus::NORMAL;
    Ringtone^ ringtone_;

    ToastNotification^ toastText;
    ToastNotification^ toastCall;
    Windows::UI::Notifications::ToastNotifier^ toaster;

    std::map<std::string, SharedCallback> callHandlers;
    std::map<std::string, SharedCallback> configurationHandlers;
    std::map<std::string, SharedCallback> presenceHandlers;
    std::map<std::string, SharedCallback> videoHandlers;
};
}
