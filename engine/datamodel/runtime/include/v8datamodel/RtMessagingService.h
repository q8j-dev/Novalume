#pragma once

#include "v8tree/Service.h"

namespace RBX {

extern const char* const sRtMessagingService;

// Client-owned realtime-messaging lifetime service. Networked hosts install a
// transport for their joined session; an offline Player intentionally remains
// on the native None transport while preserving the service contract used by
// current CoreScripts.
class RtMessagingService
    : public DescribedNonCreatable<RtMessagingService, Instance, sRtMessagingService>
    , public Service
{
public:
    enum TransportState
    {
        TRANSPORT_NONE,
        TRANSPORT_CONNECTING,
        TRANSPORT_CONNECTED,
        TRANSPORT_DISCONNECTED
    };

    RtMessagingService();

    TransportState getTransportState() const { return transportState; }
    bool hasTransport() const { return transportState != TRANSPORT_NONE; }

    void beginConnecting();
    void setConnected();
    void disconnect();

private:
    void setTransportState(TransportState value);

    TransportState transportState;
};

} // namespace RBX
