namespace RingClientUWP
{
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
    CONNECTED
};

public enum class LookupStatus {
    SUCCESS,
    INVALID_NAME,
    NOT_FOUND,
    ERRORR // one cannot simply use ERROR
};

public enum class ContactStatus {
    WAITING_FOR_ACTIVATION, // waiting for lookup
    READY
};

constexpr bool DEBUG_ON = false;
}