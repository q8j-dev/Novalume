#include "V8DataModel/VideoFrame.h"

#include "V8DataModel/ContentProvider.h"
#include "audio/SoundService.h"
#include "media/MediaPlayer.h"
#include "GfxBase/MediaTextureProvider.h"
#include "Util/StandardOut.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace RBX {

const char* const sVideoFrame = "VideoFrame";

namespace Reflection {
template<> EnumDesc<RBX::InternalVideoUsage>::EnumDesc()
    : EnumDescriptor("InternalVideoUsage")
{
    addPair(RBX::INTERNAL_VIDEO_USAGE_NONE, "None");
    addPair(RBX::INTERNAL_VIDEO_USAGE_WATCH_PAGE, "WatchPage");
}
}

REFLECTION_BEGIN();
static Reflection::PropDescriptor<VideoFrame, ContentId> prop_Video(
    "Video", category_Data, &VideoFrame::getVideo, &VideoFrame::setVideo);
static Reflection::PropDescriptor<VideoFrame, Content> prop_VideoContent(
    "VideoContent", category_Data, &VideoFrame::getVideoContent, &VideoFrame::setVideoContent);
static Reflection::PropDescriptor<VideoFrame, bool> prop_Playing(
    "Playing", category_Data, &VideoFrame::getPlaying, &VideoFrame::setPlaying);
static Reflection::PropDescriptor<VideoFrame, bool> prop_Looped(
    "Looped", category_Data, &VideoFrame::getLooped, &VideoFrame::setLooped);
static Reflection::PropDescriptor<VideoFrame, float> prop_Volume(
    "Volume", category_Data, &VideoFrame::getVolume, &VideoFrame::setVolume);
static Reflection::PropDescriptor<VideoFrame, double> prop_TimePosition(
    "TimePosition", category_Data, &VideoFrame::getTimePosition, &VideoFrame::setTimePosition);
static Reflection::PropDescriptor<VideoFrame, double> prop_TimeLength(
    "TimeLength", category_Data, &VideoFrame::getTimeLength, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<VideoFrame, bool> prop_IsLoaded(
    "IsLoaded", category_Data, &VideoFrame::getIsLoaded, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<VideoFrame, Vector2> prop_Resolution(
    "Resolution", category_Data, &VideoFrame::getResolution, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<VideoFrame, Vector2> prop_MaximumResolution(
    "MaximumResolution", category_Data, &VideoFrame::getMaximumResolution,
    &VideoFrame::setMaximumResolution);
static Reflection::EnumPropDescriptor<VideoFrame, InternalVideoUsage> prop_InternalVideoUsage(
    "InternalVideoUsage", category_Data, &VideoFrame::getInternalVideoUsage,
    &VideoFrame::setInternalVideoUsage);
static Reflection::BoundFuncDesc<VideoFrame, void()> func_Play(
    &VideoFrame::play, "Play", Security::None);
static Reflection::BoundFuncDesc<VideoFrame, void()> func_Pause(
    &VideoFrame::pause, "Pause", Security::None);
static Reflection::EventDesc<VideoFrame, void()> event_Loaded(&VideoFrame::loadedSignal, "Loaded");
static Reflection::EventDesc<VideoFrame, void()> event_Played(&VideoFrame::playedSignal, "Played");
static Reflection::EventDesc<VideoFrame, void()> event_Paused(&VideoFrame::pausedSignal, "Paused");
static Reflection::EventDesc<VideoFrame, void()> event_DidLoop(&VideoFrame::didLoopSignal, "DidLoop");
static Reflection::EventDesc<VideoFrame, void()> event_Ended(&VideoFrame::endedSignal, "Ended");
REFLECTION_END();

VideoFrame::VideoFrame()
    : DescribedCreatable<VideoFrame, GuiObject, sVideoFrame>(sVideoFrame, false)
    , IStepped(StepType_Render)
    , playing(false)
    , looped(false)
    , volume(1.0f)
    , pendingTimePosition(0.0)
    , maximumResolution(Vector2::zero())
    , internalVideoUsage(INTERNAL_VIDEO_USAGE_NONE)
    , audioEngine(NULL)
    , contentGeneration(0)
    , frameGeneration(0)
{
}

VideoFrame::~VideoFrame()
{
    closeMedia();
}

void VideoFrame::closeMedia()
{
    if (player && audioEngine)
        player->close(*audioEngine);
    player.reset();
    frame.reset();
}

void VideoFrame::loadMedia()
{
    closeMedia();
    const std::uint64_t generation = ++contentGeneration;

    if (videoContent.getSourceType() == CONTENT_SOURCE_OBJECT)
    {
        const ContentDataSource* source = dynamic_cast<const ContentDataSource*>(videoContent.getObject().get());
        std::vector<std::uint8_t> value;
        if (!source || !source->readContent(value) || value.empty())
        {
            StandardOut::singleton()->printf(MESSAGE_WARNING,
                "VideoFrame object content does not provide readable video data");
            return;
        }
        startMedia(std::make_shared<const std::vector<std::uint8_t>>(std::move(value)));
        return;
    }
    if (videoContent.getSourceType() == CONTENT_SOURCE_OPAQUE)
    {
        if (videoContent.getOpaque() && !videoContent.getOpaque()->getBytes().empty())
            startMedia(std::make_shared<const std::vector<std::uint8_t>>(
                videoContent.getOpaque()->getBytes()));
        return;
    }

    const ContentId selected = videoContent.getSourceType() == CONTENT_SOURCE_URI
        ? ContentId(videoContent.getUri()) : video;
    if (selected.isNull() || !ServiceProvider::findServiceProvider(this))
        return;

    ContentProvider* provider = ServiceProvider::create<ContentProvider>(this);
    provider->getContent(selected, ContentProvider::PRIORITY_GUI,
        [weak = weak_from(this), generation](AsyncHttpQueue::RequestResult result,
            std::istream* stream, shared_ptr<const std::string>, shared_ptr<std::exception> error) {
            shared_ptr<VideoFrame> self = weak.lock();
            if (!self || self->contentGeneration != generation)
                return;
            if (result != AsyncHttpQueue::Succeeded || !stream)
            {
                StandardOut::singleton()->printf(MESSAGE_WARNING,
                    "VideoFrame failed to load media: %s",
                    error ? error->what() : "content request failed");
                return;
            }
            auto bytes = std::make_shared<std::vector<std::uint8_t>>(
                std::istreambuf_iterator<char>(*stream), std::istreambuf_iterator<char>());
            if (bytes->empty())
                return;
            self->startMedia(bytes);
        }, AsyncHttpQueue::AsyncWrite, "Video");
}

void VideoFrame::startMedia(std::shared_ptr<const std::vector<std::uint8_t>> bytes)
{
    player = std::make_unique<Media::Player>(std::move(bytes));
    player->setLooped(looped);
    if (audioEngine)
    {
        player->setVolume(volume, *audioEngine);
        if (pendingTimePosition > 0)
            player->seek(pendingTimePosition, *audioEngine);
        player->setPlaying(playing, *audioEngine);
    }
}

void VideoFrame::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    if (oldProvider)
        closeMedia();
    Super::onServiceProvider(oldProvider, newProvider);
    onServiceProviderIStepped(oldProvider, newProvider);
    audioEngine = newProvider
        ? &ServiceProvider::create<Soundscape::SoundService>(this)->getAudioEngine()
        : NULL;
    if (newProvider && (!video.isNull() || !videoContent.empty()))
        loadMedia();
}

void VideoFrame::onStepped(const Stepped& event)
{
    if (!player || !audioEngine)
        return;
    const Media::PlaybackEvents events = player->update(*audioEngine, event.gameStep);
    if (events.loaded)
    {
        raisePropertyChanged(prop_IsLoaded);
        raisePropertyChanged(prop_TimeLength);
        raisePropertyChanged(prop_Resolution);
        loadedSignal();
    }
    if (events.frameChanged)
    {
        frame = player->currentFrame();
        ++frameGeneration;
    }
    if (events.looped)
        didLoopSignal();
    if (events.ended)
    {
        playing = false;
        raisePropertyChanged(prop_Playing);
        raisePropertyChanged(prop_TimePosition);
        endedSignal();
    }
    if (events.failed)
        StandardOut::singleton()->printf(MESSAGE_WARNING,
            "VideoFrame decode failed: %s", player->failure().c_str());
}

void VideoFrame::setVideo(ContentId value)
{
    if (video == value)
        return;
    video = value;
    raisePropertyChanged(prop_Video);
    if (videoContent.empty())
        loadMedia();
}

void VideoFrame::setVideoContent(Content value)
{
    if (videoContent == value)
        return;
    videoContent = value;
    raisePropertyChanged(prop_VideoContent);
    loadMedia();
}

void VideoFrame::setPlaying(bool value)
{
    if (playing == value)
        return;
    playing = value;
    if (player && audioEngine)
        player->setPlaying(value, *audioEngine);
    raisePropertyChanged(prop_Playing);
    if (value)
        playedSignal();
    else
        pausedSignal();
}

void VideoFrame::setLooped(bool value)
{
    if (looped == value)
        return;
    looped = value;
    if (player)
        player->setLooped(value);
    raisePropertyChanged(prop_Looped);
}

void VideoFrame::setVolume(float value)
{
    value = std::max(value, 0.0f);
    if (volume == value)
        return;
    volume = value;
    if (player && audioEngine)
        player->setVolume(value, *audioEngine);
    raisePropertyChanged(prop_Volume);
}

double VideoFrame::getTimePosition() const
{
    return player ? player->timePosition() : pendingTimePosition;
}

void VideoFrame::setTimePosition(double value)
{
    value = std::max(value, 0.0);
    pendingTimePosition = value;
    if (player && audioEngine)
        player->seek(value, *audioEngine);
    raisePropertyChanged(prop_TimePosition);
}

double VideoFrame::getTimeLength() const
{
    return player ? player->timeLength() : 0.0;
}

bool VideoFrame::getIsLoaded() const
{
    return player && player->loaded() && frame;
}

Vector2 VideoFrame::getResolution() const
{
    if (!player)
        return Vector2::zero();
    const Media::StreamInfo info = player->info();
    return Vector2(static_cast<float>(info.width), static_cast<float>(info.height));
}

void VideoFrame::setMaximumResolution(Vector2 value)
{
    value.x = std::max(value.x, 0.0f);
    value.y = std::max(value.y, 0.0f);
    if (maximumResolution != value)
    {
        maximumResolution = value;
        raisePropertyChanged(prop_MaximumResolution);
    }
}

void VideoFrame::setInternalVideoUsage(InternalVideoUsage value)
{
    if (internalVideoUsage != value)
    {
        internalVideoUsage = value;
        raisePropertyChanged(prop_InternalVideoUsage);
    }
}

void VideoFrame::play()
{
    setPlaying(true);
}

void VideoFrame::pause()
{
    setPlaying(false);
}

void VideoFrame::render2d(Adorn* adorn)
{
    MediaTextureProvider* provider = dynamic_cast<MediaTextureProvider*>(adorn);
    if (!provider || !frame)
        return;
    TextureProxyBaseRef texture = provider->requestMediaTexture(this, frameGeneration,
        frame->width, frame->height, frame->rgba.data(), frame->rgba.size());
    if (!texture)
        return;
    adorn->setTexture(0, texture);
    const Color4 color = applyCanvasGroup(Color4(1.0f, 1.0f, 1.0f, 1.0f));
    GuiObject* clippingObject = firstAncestorClipping();
    if (clippingObject && getAbsoluteRotation().empty())
        adorn->rect2d(getRect2D(), Vector2(0, 0), Vector2(1, 1), color,
            clippingObject->getClippedRect());
    else
        adorn->rect2d(getRect2D(), Vector2(0, 0), Vector2(1, 1), color,
            getAbsoluteRotation());
    adorn->setTexture(0, TextureProxyBaseRef());
}

void VideoFrame::renderBackground2d(Adorn* adorn)
{
    if (getBackgroundTransparency() < 1.0f)
        render2dImpl(adorn, getRenderBackgroundColor4());
}

} // namespace RBX
