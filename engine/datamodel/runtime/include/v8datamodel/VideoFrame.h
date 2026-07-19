#pragma once

#include "V8DataModel/GuiObject.h"
#include "Util/ContentId.h"
#include "Util/Content.h"
#include "Util/SteppedInstance.h"

#include <cstdint>
#include <memory>

namespace RBX::Audio {
class Engine;
}
namespace RBX::Media {
class Player;
struct VideoFrame;
}

namespace RBX {

extern const char* const sVideoFrame;

enum InternalVideoUsage
{
    INTERNAL_VIDEO_USAGE_NONE = 0,
    INTERNAL_VIDEO_USAGE_WATCH_PAGE = 1,
};

class VideoFrame
    : public DescribedCreatable<VideoFrame, GuiObject, sVideoFrame>
    , public IStepped
{
private:
    typedef DescribedCreatable<VideoFrame, GuiObject, sVideoFrame> Super;

    ContentId video;
    Content videoContent;
    bool playing;
    bool looped;
    float volume;
    double pendingTimePosition;
    Vector2 maximumResolution;
    InternalVideoUsage internalVideoUsage;
    std::unique_ptr<Media::Player> player;
    std::shared_ptr<const Media::VideoFrame> frame;
    Audio::Engine* audioEngine;
    std::uint64_t contentGeneration;
    std::uint64_t frameGeneration;

    void loadMedia();
    void closeMedia();
    void startMedia(std::shared_ptr<const std::vector<std::uint8_t>> bytes);

protected:
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
    /*override*/ void onStepped(const Stepped& event);
    /*override*/ void render2d(Adorn* adorn);
    /*override*/ void renderBackground2d(Adorn* adorn);

public:
    VideoFrame();
    ~VideoFrame();

    ContentId getVideo() const { return video; }
    void setVideo(ContentId value);
    Content getVideoContent() const { return videoContent; }
    void setVideoContent(Content value);
    bool getPlaying() const { return playing; }
    void setPlaying(bool value);
    bool getLooped() const { return looped; }
    void setLooped(bool value);
    float getVolume() const { return volume; }
    void setVolume(float value);
    double getTimePosition() const;
    void setTimePosition(double value);
    double getTimeLength() const;
    bool getIsLoaded() const;
    Vector2 getResolution() const;
    Vector2 getMaximumResolution() const { return maximumResolution; }
    void setMaximumResolution(Vector2 value);
    InternalVideoUsage getInternalVideoUsage() const { return internalVideoUsage; }
    void setInternalVideoUsage(InternalVideoUsage value);

    void play();
    void pause();

    rbx::signal<void()> loadedSignal;
    rbx::signal<void()> playedSignal;
    rbx::signal<void()> pausedSignal;
    rbx::signal<void()> didLoopSignal;
    rbx::signal<void()> endedSignal;
};

} // namespace RBX
