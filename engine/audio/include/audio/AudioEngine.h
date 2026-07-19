#pragma once

#include <cstddef>
#include <cstdint>
#include <array>
#include <filesystem>
#include <memory>
#include <span>
#include <vector>

namespace RBX::Audio {

struct Vector3
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct ClipHandle
{
    std::uint32_t index = UINT32_MAX;
    std::uint32_t generation = 0;
    explicit operator bool() const noexcept { return index != UINT32_MAX; }
};

struct VoiceHandle
{
    std::uint32_t index = UINT32_MAX;
    std::uint32_t generation = 0;
    explicit operator bool() const noexcept { return index != UINT32_MAX; }
};

struct BusHandle
{
    std::uint32_t index = UINT32_MAX;
    std::uint32_t generation = 0;
    explicit operator bool() const noexcept { return index != UINT32_MAX; }
};

struct EngineConfig
{
    std::uint32_t sampleRate = 48000;
    std::uint32_t channels = 2;
    std::uint64_t maxClipFrames = 48000ULL * 60ULL * 10ULL;
    std::size_t maxEncodedBytes = 64ULL * 1024ULL * 1024ULL;
    std::uint32_t maxVoices = 128;
};

struct PcmClip
{
    std::uint32_t sampleRate = 48000;
    std::uint32_t channels = 1;
    std::vector<float> samples;
};

enum class AttenuationModel
{
    Inverse,
    Linear,
};

struct AttenuationPoint
{
    float distance = 0.0f;
    float gain = 1.0f;
};

enum class VoiceEffectType : std::uint8_t
{
    Distortion,
    Tremolo,
    Chorus,
    Flanger,
    Compressor,
    Gate,
    Limiter,
    Equalizer,
    Filter,
};

// Fixed-size effect descriptors keep graph updates and the real-time callback
// allocation-free while preserving the authored wire order.
struct VoiceEffect
{
    VoiceEffectType type = VoiceEffectType::Distortion;
    std::array<float, 7> parameters{};
};

struct VoiceParameters
{
    float volume = 1.0f;
    float pitch = 1.0f;
    bool looping = false;
    bool spatial = false;
    std::uint32_t listenerIndex = 0;
    float minDistance = 1.0f;
    float maxDistance = 10000.0f;
    float rolloff = 1.0f;
    float dopplerFactor = 1.0f;
    std::array<float, 32> distortionLevels{};
    std::uint32_t distortionCount = 0;
    std::array<VoiceEffect, 32> effects{};
    std::uint32_t effectCount = 0;
    AttenuationModel attenuation = AttenuationModel::Inverse;
    std::int32_t priority = 0;
    std::uint64_t rangeBeginFrame = 0;
    std::uint64_t rangeEndFrame = UINT64_MAX;
    std::uint64_t loopBeginFrame = 0;
    std::uint64_t loopEndFrame = UINT64_MAX;
    double startMixerTimeSeconds = -1.0;
    BusHandle bus;
    Vector3 position;
    Vector3 direction{0.0f, 0.0f, -1.0f};
    Vector3 velocity;
};

struct ListenerState
{
    Vector3 position;
    Vector3 direction{0.0f, 0.0f, -1.0f};
    Vector3 up{0.0f, 1.0f, 0.0f};
    Vector3 velocity;
    std::vector<AttenuationPoint> distanceAttenuationCurve;
    std::vector<AttenuationPoint> angleAttenuationCurve;
};

struct ReverbParameters
{
    bool enabled = false;
    float mix = 0.0f;
    float decay = 0.5f;
    float damping = 0.25f;
    float roomSize = 0.5f;
};

struct EqualizerParameters
{
    bool enabled = false;
    float lowGainDb = 0.0f;
    float midGainDb = 0.0f;
    float highGainDb = 0.0f;
};

struct CompressorParameters
{
    bool enabled = false;
    float thresholdDb = -12.0f;
    float ratio = 4.0f;
    float attackSeconds = 0.01f;
    float releaseSeconds = 0.1f;
    float makeupGainDb = 0.0f;
};

struct EchoParameters
{
    bool enabled = false;
    float delaySeconds = 0.25f;
    float feedback = 0.35f;
    float mix = 0.2f;
};

// FMOD-free engine contract. Control methods are called outside the real-time
// callback. mix() performs no owned allocation and is the deterministic path
// used by offline tests and, later, the platform playback callback.
class Engine
{
public:
    explicit Engine(const EngineConfig& config);
    ~Engine();

    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    ClipHandle createClip(PcmClip clip);
    ClipHandle createEncodedClip(std::span<const std::byte> encodedData);
    ClipHandle createStreamingClip(const std::filesystem::path& path);
    ClipHandle createQueuedClip(std::uint32_t sampleRate, std::uint32_t channels,
        std::uint64_t lengthFrames, std::uint32_t capacityFrames);
    std::uint32_t submitQueuedFrames(ClipHandle clip, std::span<const float> samples);
    std::uint32_t queuedFrameCapacity(ClipHandle clip) const;
    std::uint32_t queuedFramesAvailable(ClipHandle clip) const;
    bool resetQueuedClip(ClipHandle clip);
    bool destroyClip(ClipHandle clip);

    BusHandle createBus(float volume = 1.0f);
    bool destroyBus(BusHandle bus);
    bool setBusVolume(BusHandle bus, float volume);
    float busVolume(BusHandle bus) const;

    void setReverb(const ReverbParameters& parameters) noexcept;
    ReverbParameters reverb() const noexcept;
    void setEqualizer(const EqualizerParameters& parameters) noexcept;
    EqualizerParameters equalizer() const noexcept;
    void setCompressor(const CompressorParameters& parameters) noexcept;
    CompressorParameters compressor() const noexcept;
    void setEcho(const EchoParameters& parameters) noexcept;
    EchoParameters echo() const noexcept;

    VoiceHandle play(ClipHandle clip, const VoiceParameters& parameters = {});
    bool destroyVoice(VoiceHandle voice);
    bool pause(VoiceHandle voice);
    bool resume(VoiceHandle voice);
    bool stop(VoiceHandle voice);
    bool seekFrames(VoiceHandle voice, std::uint64_t frame);
    bool setVoiceTransform(VoiceHandle voice, Vector3 position, Vector3 velocity);
    bool setVoiceDirection(VoiceHandle voice, Vector3 direction);
    bool setVoiceSpatialModel(VoiceHandle voice, float minDistance, float maxDistance,
        float rolloff, float dopplerFactor, AttenuationModel attenuation);
    bool setVoiceAttenuationCurve(VoiceHandle voice,
        std::span<const AttenuationPoint> curve);
    bool setVoiceAngleAttenuationCurve(VoiceHandle voice,
        std::span<const AttenuationPoint> curve);
    bool setVoiceVolume(VoiceHandle voice, float volume);
    bool setVoicePitch(VoiceHandle voice, float pitch);
    bool setVoiceEffects(VoiceHandle voice,
        std::span<const VoiceEffect> effects);
    bool setVoiceDistortion(VoiceHandle voice, std::span<const float> levels);
    bool setVoiceLooping(VoiceHandle voice, bool looping);
    bool scheduleVoiceStop(VoiceHandle voice, double mixerTimeSeconds);
    bool cancelVoiceStop(VoiceHandle voice);
    bool isPlaying(VoiceHandle voice) const;
    bool isPaused(VoiceHandle voice) const;
    bool isFinished(VoiceHandle voice) const;
    std::uint64_t positionFrames(VoiceHandle voice) const;
    std::uint64_t lengthFrames(VoiceHandle voice) const;
    std::uint64_t clipLengthFrames(ClipHandle clip) const;
    std::uint32_t clipSampleRate(ClipHandle clip) const;

    void setListener(const ListenerState& listener);
    void setGraphListener(const ListenerState& listener);
    void setMasterVolume(float volume);
    float masterVolume() const noexcept;
    void setMuted(bool muted);
    bool muted() const noexcept;
    std::uint32_t activeVoiceCount() const noexcept;
    void startOutputDevice();
    void stopOutputDevice() noexcept;
    void restartOutputDevice();
    void suspend() noexcept;
    void resume();
    bool suspended() const noexcept;
    bool outputDeviceStarted() const noexcept;
    bool outputDeviceInterrupted() const noexcept;
    std::uint64_t outputDeviceEventSerial() const noexcept;
    double mixerTimeSeconds() const noexcept;
    bool mix(std::span<float> interleavedOutput) noexcept;

    std::uint32_t sampleRate() const noexcept;
    std::uint32_t channels() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};

} // namespace RBX::Audio
