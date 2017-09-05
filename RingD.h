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

#pragma once

#include "MainPage.xaml.h"
#include "Ringtone.h"
#include "Utils.h"
#include "ThreadUtils.h"

#include <dring.h>

using namespace concurrency;

using namespace Windows::UI::Notifications;
using namespace Windows::Data::Xml::Dom;
using namespace Windows::UI::ViewManagement;

using namespace std::placeholders;

namespace RingClientUWP
{

/* DRing delegates */
/* nb: <PeerHold> forwards to <StateChange> */
delegate void IncomingCall(String^ accountId, String^ callId, String^ from);
delegate void StateChange(String^ callId, CallStatus state, int code);
delegate void SmartInfo(Map<String^, String^>^ smartInfo);
delegate void AudioMuted(String^ callId, bool state);
delegate void VideoMuted(String^ callId, bool state);
delegate void IncomingMessage(String^ callId, String^ from, Map<String^, String^>^ payload);
delegate void IncomingAccountMessage(String^ accountId, String^ from, Map<String^, String^>^ payload);
delegate void AccountMessageStatusChanged(String^ accountId, uint64_t messageId, String^ to, int state);
delegate void RegistrationStateChanged(String^ accountId, String^ state, int detailsCode, String^ detailsStr);
delegate void AccountsChanged();
delegate void NameRegistrationEnded(String^ accountId, int state, String^ name);
delegate void KnownDevicesChanged(String^ accountId, Map<String^, String^>^ devices);
delegate void ExportOnRingEnded(String^ accountId, int status, String^ pin);
delegate void RegisteredNameFound(String^ accountId, LookupStatus status, String^ address, String^ name);
delegate void VolatileDetailsChanged(String^ accountId, Map<String^, String^>^ details);
delegate void IncomingTrustRequest(String^ accountId, String^ from, String^ payload, time_t received);
delegate void ContactAdded(String^ accountId, String^ uri, bool confirmed);
delegate void DeviceRevocationEnded(String^ accountId, String^ device, DeviceRevocationResult status);
delegate void NewBuddyNotification(String^ accountId, String^ uri, int status, String^ lineStatus);
delegate void DecodingStarted(String^ callId, String^ shmPath, int width, int height, bool isMixer);
delegate void DecodingStopped(String^ callId, String^ shmPath, bool isMixer);
delegate void ParametersChanged(String^ device);
delegate void StartCapture(String^ device);
delegate void StopCapture();

/* Client delegates */
delegate void SelectAccountItemIndex(int index);
delegate void CallPlaced(String^ callId);
delegate void DevicesListRefreshed(Map<String^, String^>^ deviceMap);
delegate void SummonWizard();
delegate void AccountUpdated(Account^ account);
delegate void IncomingVideoMuted(String^ callId, bool state);
delegate void FinishCaptureDeviceEnumeration();
delegate void RegistrationStateErrorGeneric(const std::string& accountId);
delegate void RegistrationStateRegistered(const std::string& accountId);
delegate void RegistrationStateUnregistered(const std::string& accountId);
delegate void RegistrationStateTrying(const std::string& accountId);
delegate void SetOverlayStatusText(String^ statusText, String^ color);

delegate void FullScreenToggled(bool state);
delegate void WindowResized(float width, float height);
delegate void NetworkChanged();
delegate void MessageDataLoaded();

delegate void MessageStatusUpdated(String^ messageId, int status);


delegate void VCardUpdated(Contact^ owner);
delegate void ShareRequested();
delegate void NameRegistered(bool status, String^ accountId);

public ref class RingD sealed
{
/* singleton */
private:
    RingD();

internal:
    static property RingD^ instance {
        RingD^ get() {
            static RingD^ instance_ = ref new RingD();
            return instance_;
        }
    }

public:
    using AccountDetails = std::map<std::string, std::string>;
    using AccountDetailsBlob = std::map<std::string, AccountDetails>;
    using SharedCallback = std::shared_ptr<DRing::CallbackWrapperBase>;
    using SignalHandlerMap = std::map<std::string, SharedCallback>;

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

    property bool isCtrlPressed;
    property bool isShiftPressed;

internal:
    void init();
    void startDaemon();

    void addContactFromDaemon(String^ accountId, Map<String^, String^>^ details);
    void cancelOutGoingCall(String^ callId);
    void raiseWindowResized(float width, float height);
    void raiseShareRequested();
    std::string getLocalFolder();
    AccountDetailsBlob getAllAccountDetails();
    void parseAccountDetails(String^ accountId, const AccountDetails& allAccountDetails);
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
    LookupStatus translateLookupStatus(int status);
    DeviceRevocationResult translateDeviceRevocationResult(int status);
    String^ getUserName();
    void startSmartInfo(int refresh);
    void stopSmartInfo();
    void handleIncomingMessage(String^ from, Map<String^, String^>^ payloads);

    void toggleFullScreen();
    void setWindowedMode();
    void setFullScreenMode();
    void connectivityChanged();
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
    String^ getRegisteredName(String^ accountId);
    void removeContact(const std::string & accountId, const std::string& uri);
    void sendContactRequest(const std::string& accountId, const std::string& uri, const std::string& payload);
    void raiseMessageDataLoaded();
    void raiseVCardUpdated(Contact^ contact);
    void revokeDevice(const std::string& accountId, const std::string& password, const std::string& deviceId);
    void showLoadingOverlay(String^ text, String^ color);
    void hideLoadingOverlay(String^ text, String^ color, int delayInMilliseconds = 2000);
    void onAccountAdded(String^ accountId);
    void onAccountUpdated();
    void OnaccountDeleted();
    void selectAccount(int index);
    void selectAccount(String^ accountId);
    void ShowCallToast(bool background, String^ callId, String^ name = nullptr);
    void ShowIMToast(bool background, String^ from, String^ payload, String^ name = nullptr);
    void HideToast(ToastNotification^ toast);

    std::map<String^, std::function<void(String^ username)>> unpoppedToasts;

    /*
    * DRing signaling
    */

    /* events (signals) */
    event IncomingCall^ incomingCall;
    event StateChange^ stateChange;
    event IncomingAccountMessage^ incomingAccountMessage;
    event IncomingMessage^ incomingMessage;
    event AccountMessageStatusChanged^ accountMessageStatusChanged;
    event RegistrationStateChanged^ registrationStateChanged;
    event AccountsChanged^ accountsChanged;
    event NameRegistrationEnded^ nameRegistrationEnded;
    event KnownDevicesChanged^ knownDevicesChanged;
    event ExportOnRingEnded^ exportOnRingEnded;
    event RegisteredNameFound^ registeredNameFound;
    event VolatileDetailsChanged^ volatileDetailsChanged;
    event IncomingTrustRequest^ incomingTrustRequest;
    event ContactAdded^ contactAdded;
    event DeviceRevocationEnded^ deviceRevocationEnded;
    event DecodingStarted^ decodingStarted;
    event DecodingStopped^ decodingStopped;
    event ParametersChanged^ parametersChanged;
    event StartCapture^ startCapture;
    event StopCapture^ stopCapture;

    /* delegates (slots) */
    void onIncomingCall(String^ accountId, String^ callId, String^ from);
    void onStateChange(String ^callId, CallStatus state, int code);
    void onIncomingMessage(String^ callId, String^ from, Map<String^, String^>^ payloads);
    void onIncomingAccountMessage(String^ accountId, String^ from, Map<String^, String^>^ payloads);
    void onAccountMessageStatusChanged(String^ accountId, uint64_t messageId, String^ to, int state);
    void onRegistrationStateChanged(String^ accountId, String^ state, int detailsCode, String^ detailsStr);
    void onAccountsChanged();
    void onNameRegistrationEnded(String^ accountId, int state, String^ name);
    void onKnownDevicesChanged(String^ accountId, Map<String^, String^>^ devices);
    void onExportOnRingEnded(String^ accountId, int status, String^ pin);
    void onIncomingTrustRequest(String^ accountId, String^ from, String^ payload, time_t received);
    void onContactAdded(String^ accountId, String^ uri, bool confirmed);
    void onDeviceRevocationEnded(String^ accountId, String^ device, DeviceRevocationResult status);
    void onDecodingStarted(String^ callId, String^ shmPath, int width, int height, bool isMixer);
    void onDecodingStopped(String^ callId, String^ shmPath, bool isMixer);
    void onParametersChanged(String^ device);
    void onStartCapture(String^ device);
    void onStopCapture();

    /*
    * Client signaling
    */

    /* events (signals) */
    event SelectAccountItemIndex^ selectAccountItemIndex;
    event CallPlaced^ callPlaced;
    event DevicesListRefreshed^ devicesListRefreshed;
    event SummonWizard^ summonWizard;
    event AccountUpdated^ accountUpdated;
    event IncomingVideoMuted^ incomingVideoMuted;
    event FinishCaptureDeviceEnumeration^ finishCaptureDeviceEnumeration;
    event RegistrationStateErrorGeneric^ registrationStateErrorGeneric;
    event RegistrationStateRegistered^ registrationStateRegistered;
    event RegistrationStateUnregistered^ registrationStateUnregistered;
    event RegistrationStateTrying^ registrationStateTrying;
    event SetOverlayStatusText^ setOverlayStatusText;
    event AudioMuted^ audioMuted;
    event VideoMuted^ videoMuted;
    event FullScreenToggled^ fullScreenToggled;
    event WindowResized^ windowResized;
    event NetworkChanged^ networkChanged;
    event MessageDataLoaded^ messageDataLoaded;
    event SmartInfo^ smartInfo;
    event MessageStatusUpdated^ messageStatusUpdated;
    event NewBuddyNotification^ newBuddyNotification;
    event VCardUpdated^ vCardUpdated;
    event ShareRequested^ shareRequested;
    event NameRegistered^ nameRegistered;

private:
    void InternetConnectionChanged(Object^ sender);

    void initDaemon(int flags);
    void registerCallbacks();
    void registerCallHandlers();
    void registerConfigurationHandlers();
    void registerPresenceHandlers();
    void registerVideoHandlers();
    void connectDelegates();
    void deinit();

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
    Utils::Threading::task_queue tasks_;
    Ringtone^ ringtone_;

    ToastNotification^ toastText_;
    ToastNotification^ toastCall_;
    ToastNotifier^ toaster_;

    SignalHandlerMap callHandlers;
    SignalHandlerMap configurationHandlers;
    SignalHandlerMap presenceHandlers;
    SignalHandlerMap videoHandlers;
};
}
