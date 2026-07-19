#include "V8DataModel/VideoService.h"

namespace RBX {

const char* const sVideoService = "VideoService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<VideoService, bool()>
    funcGameStreamingEnabled(&VideoService::gameStreamingEnabled,
        "GameStreamingEnabled", Security::RobloxScript);
static Reflection::EventDesc<VideoService, void()>
    eventGameStreamingResolutionReady(
        &VideoService::gameStreamingResolutionReadySignal,
        "GameStreamingResolutionReady", Security::RobloxScript);
REFLECTION_END();

VideoService::VideoService()
    : Service(true)
    , streamingEnabled(false)
{
    setName(sVideoService);
    setRobloxLocked(true);
}

void VideoService::setGameStreamingEnabled(bool value)
{
    if (streamingEnabled == value)
        return;
    streamingEnabled = value;
    if (streamingEnabled)
        gameStreamingResolutionReadySignal();
}

} // namespace RBX
