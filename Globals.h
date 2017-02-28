namespace RingClientUWP
{

namespace ViewModel
{
namespace NotifyStrings
{
const std::vector<std::string> notifySmartPanelItem = {
    "_isSelected",
    "_contactStatus",
    "_lastTime",
    "_presenceStatus",
    "_displayName",
    "_name",
    "_bestName",
    "_bestName2",
    "notificationNewMessage",
    "_unreadMessages",
    "_unreadContactRequest",
    "_trustStatus" };
const std::vector<std::string> notifyContactRequestItem = {
    "_isSelected" };
const std::vector<std::string> notifyAccountItem = { "nothing" };
    /*"_isSelected",
    "_unreadMessages",
    "_unreadContactRequests",
    "_sipUsername",
    "_sipHostname",
    "_bestName",
    "_bestName2",
    "name_",
    "ringID_",
    "_registrationState",
    "_upnpState",
    "_autoAnswer",
    "_dhtPublicInCalls",
    "_turnEnabled",
    "accountType_",
    "accountID_",
    "_deviceId",
    "_deviceName",
    "_active" };*/
const std::vector<std::string> notifyConversation = { "" };
}
}

/* public enumerations. */
public enum class CallStatus {
    NONE,
    OUTGOING_REQUESTED,
    INCOMING_RINGING,
    OUTGOING_RINGING,
    SEARCHING,
    IN_PROGRESS,
    PAUSED,
    PEER_PAUSED,
    ENDED,
    TERMINATING,
    CONNECTED,
    AUTO_ANSWERING
};

public enum class DeviceRevocationResult {
    SUCCESS,
    INVALID_PASSWORD,
    INVALID_CERTIFICATE
};

public enum class TrustStatus {
    UNKNOWN,
    INCOMING_CONTACT_REQUEST,
    INGNORED,
    BLOCKED,
    CONTACT_REQUEST_SENT,
    TRUSTED
};

public enum class RegistrationState {
    UNKNOWN,
    TRYING,
    REGISTERED,
    UNREGISTERED
};

public enum class LookupStatus {
    SUCCESS,
    INVALID_NAME,
    NOT_FOUND,
    ERRORR // one cannot simply use ERROR
};

public enum class ContactStatus {
    WAITING_FOR_ACTIVATION,
    READY
};

/* public enumerations. */
constexpr bool DEBUG_ON = true;

}