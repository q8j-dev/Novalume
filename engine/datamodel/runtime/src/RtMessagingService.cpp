#include "v8datamodel/RtMessagingService.h"

namespace RBX {

const char* const sRtMessagingService = "RtMessagingService";

RtMessagingService::RtMessagingService()
    : Service(true)
    , transportState(TRANSPORT_NONE)
{
    setName(sRtMessagingService);
    setRobloxLocked(true);
}

void RtMessagingService::setTransportState(TransportState value)
{
    if (transportState == value)
        return;
    transportState = value;
}

void RtMessagingService::beginConnecting()
{
    setTransportState(TRANSPORT_CONNECTING);
}

void RtMessagingService::setConnected()
{
    setTransportState(TRANSPORT_CONNECTED);
}

void RtMessagingService::disconnect()
{
    setTransportState(transportState == TRANSPORT_NONE
        ? TRANSPORT_NONE
        : TRANSPORT_DISCONNECTED);
}

} // namespace RBX
