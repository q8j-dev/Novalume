#pragma once

#include "v8tree/Service.h"

#include "rbx/signal.h"

namespace RBX {

extern const char* const sVideoService;

class VideoService
    : public DescribedNonCreatable<VideoService, Instance, sVideoService>
    , public Service
{
public:
    VideoService();

    bool gameStreamingEnabled() { return streamingEnabled; }
    void setGameStreamingEnabled(bool value);

    rbx::signal<void()> gameStreamingResolutionReadySignal;

private:
    bool streamingEnabled;
};

} // namespace RBX
