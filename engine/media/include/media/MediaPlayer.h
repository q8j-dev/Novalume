#pragma once

#include "media/MediaDecoder.h"

#include <filesystem>
#include <memory>

namespace RBX::Audio {
class Engine;
}

namespace RBX::Media {

struct PlaybackEvents
{
    bool loaded = false;
    bool looped = false;
    bool ended = false;
    bool frameChanged = false;
    bool failed = false;
};

class Player final
{
public:
    explicit Player(const std::filesystem::path& path);
    explicit Player(std::shared_ptr<const std::vector<std::uint8_t>> bytes);
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    PlaybackEvents update(Audio::Engine& audio, double elapsedSeconds);
    void close(Audio::Engine& audio);
    void setPlaying(bool value, Audio::Engine& audio);
    bool playing() const noexcept;
    void setLooped(bool value) noexcept;
    bool looped() const noexcept;
    void setVolume(float value, Audio::Engine& audio);
    float volume() const noexcept;
    void seek(double seconds, Audio::Engine& audio);
    double timePosition() const noexcept;
    double timeLength() const noexcept;
    bool loaded() const noexcept;
    bool failed() const noexcept;
    std::string failure() const;
    StreamInfo info() const;
    std::shared_ptr<const VideoFrame> currentFrame() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace RBX::Media
