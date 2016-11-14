namespace RingClientUWP
{
/* public enumerations. */
public enum class CallStatus {
    NONE,
    INCOMING_RINGING,
    OUTGOING_RINGING,
    SEARCHING,
    IN_PROGRESS,
    PAUSED,
    ENDED,
    TERMINATING
};

public enum class LookupStatus {
    SUCCESS,
    INVALID_NAME,
    NOT_FOUND,
    ERRORR // one cannot simply use ERROR
};

}