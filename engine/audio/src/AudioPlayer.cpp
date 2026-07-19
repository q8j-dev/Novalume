#include "audio/AudioGraph.h"

#include "audio/Sound.h"
#include "audio/SoundService.h"
#include "Reflection/Reflection.h"
#include "V8DataModel/PartInstance.h"
#include "lua/lua.hpp"

#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace RBX::Soundscape {

namespace {

struct AudioRoute
{
    AudioEmitter* emitter = nullptr;
    bool connected = false;
    float gain = 1.0f;
    std::array<Audio::VoiceEffect, 32> effects{};
    std::uint32_t effectCount = 0;
};

AudioRoute findRoute(const AudioNode* node, float gain,
    std::array<Audio::VoiceEffect, 32> effects, std::uint32_t effectCount,
    std::unordered_set<const Instance*>& visited)
{
    if (!node)
        return {};
    const Instance* nodeInstance = node->audioNodeInstance();
    if (!nodeInstance || !visited.insert(nodeInstance).second)
        return {};
    for (const std::string& outputPin : node->outputPins())
    {
        if (Instance::fastDynamicCast<AudioChannelSplitter>(nodeInstance) &&
            outputPin != "Output")
            continue;
        const boost::shared_ptr<const Instances> wires =
            node->getConnectedWires(outputPin);
        if (!wires)
            continue;
        for (const boost::shared_ptr<Instance>& instance : *wires)
        {
            Wire* wire = Instance::fastDynamicCast<Wire>(instance.get());
            if (!wire || wire->getSourceInstance() != nodeInstance ||
                wire->getSourceName() != outputPin)
                continue;
            Instance* target = wire->getTargetInstance();
            if (AudioEmitter* emitter =
                    Instance::fastDynamicCast<AudioEmitter>(target))
            {
                AudioRoute route;
                route.emitter = emitter;
                route.connected = true;
                route.gain = gain;
                route.effects = effects;
                route.effectCount = effectCount;
                return route;
            }
            if (Instance::fastDynamicCast<AudioDeviceOutput>(target))
            {
                AudioRoute route;
                route.connected = true;
                route.gain = gain;
                route.effects = effects;
                route.effectCount = effectCount;
                return route;
            }
            if (AudioFader* fader = Instance::fastDynamicCast<AudioFader>(target))
            {
                const float faderGain = fader->getBypass()
                    ? 1.0f : fader->getVolume();
                AudioRoute route = findRoute(fader, gain * faderGain,
                    effects, effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioDistortion* distortion =
                         Instance::fastDynamicCast<AudioDistortion>(target))
            {
                if (!distortion->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::Distortion;
                    effect.parameters[0] = distortion->getLevel();
                }
                AudioRoute route = findRoute(distortion, gain,
                    effects, effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioTremolo* tremolo =
                         Instance::fastDynamicCast<AudioTremolo>(target))
            {
                if (!tremolo->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::Tremolo;
                    effect.parameters = {tremolo->getDepth(), tremolo->getDuty(),
                        tremolo->getFrequency(), tremolo->getShape(),
                        tremolo->getSkew(), tremolo->getSquare(), 0.0f};
                }
                AudioRoute route = findRoute(tremolo, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioChorus* chorus =
                         Instance::fastDynamicCast<AudioChorus>(target))
            {
                if (!chorus->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::Chorus;
                    effect.parameters = {chorus->getDepth(), chorus->getMix(),
                        chorus->getRate(), 0.0f, 0.0f, 0.0f, 0.0f};
                }
                AudioRoute route = findRoute(chorus, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioFlanger* flanger =
                         Instance::fastDynamicCast<AudioFlanger>(target))
            {
                if (!flanger->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::Flanger;
                    effect.parameters = {flanger->getDepth(), flanger->getMix(),
                        flanger->getRate(), 0.0f, 0.0f, 0.0f, 0.0f};
                }
                AudioRoute route = findRoute(flanger, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioCompressor* compressor =
                         Instance::fastDynamicCast<AudioCompressor>(target))
            {
                if (!compressor->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::Compressor;
                    effect.parameters = {compressor->getAttack(),
                        compressor->getMakeupGain(), compressor->getRatio(),
                        compressor->getRelease(), compressor->getThreshold(),
                        0.0f, 0.0f};
                }
                AudioRoute route = findRoute(compressor, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioGate* gate =
                         Instance::fastDynamicCast<AudioGate>(target))
            {
                if (!gate->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::Gate;
                    const NumberRange threshold = gate->getThreshold();
                    effect.parameters = {gate->getAttack(), gate->getRelease(),
                        threshold.min, threshold.max, 0.0f, 0.0f, 0.0f};
                }
                AudioRoute route = findRoute(gate, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioLimiter* limiter =
                         Instance::fastDynamicCast<AudioLimiter>(target))
            {
                if (!limiter->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::Limiter;
                    effect.parameters = {limiter->getMaxLevel(),
                        limiter->getRelease(), 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
                }
                AudioRoute route = findRoute(limiter, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioEqualizer* equalizer =
                         Instance::fastDynamicCast<AudioEqualizer>(target))
            {
                if (!equalizer->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::Equalizer;
                    const NumberRange midRange = equalizer->getMidRange();
                    effect.parameters = {equalizer->getLowGain(),
                        equalizer->getMidGain(), equalizer->getHighGain(),
                        midRange.min, midRange.max, 0.0f, 0.0f};
                }
                AudioRoute route = findRoute(equalizer, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioFilter* filter =
                         Instance::fastDynamicCast<AudioFilter>(target))
            {
                if (!filter->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::Filter;
                    effect.parameters = {
                        static_cast<float>(filter->getFilterType()),
                        filter->getFrequency(), filter->getGain(),
                        filter->getQ(), 0.0f, 0.0f, 0.0f};
                }
                AudioRoute route = findRoute(filter, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioPitchShifter* pitchShifter =
                         Instance::fastDynamicCast<AudioPitchShifter>(target))
            {
                if (!pitchShifter->getBypass())
                {
                    if (effectCount >= effects.size())
                        continue;
                    Audio::VoiceEffect& effect = effects[effectCount++];
                    effect.type = Audio::VoiceEffectType::PitchShifter;
                    effect.parameters = {pitchShifter->getPitch(),
                        static_cast<float>(pitchShifter->getWindowSize()),
                        0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
                }
                AudioRoute route = findRoute(pitchShifter, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioChannelMixer* mixer =
                         Instance::fastDynamicCast<AudioChannelMixer>(target))
            {
                if (wire->getTargetName() != "Input")
                    continue;
                AudioRoute route = findRoute(mixer, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
            else if (AudioChannelSplitter* splitter =
                         Instance::fastDynamicCast<AudioChannelSplitter>(target))
            {
                if (wire->getTargetName() != "Input")
                    continue;
                AudioRoute route = findRoute(splitter, gain, effects,
                    effectCount, visited);
                if (route.connected)
                    return route;
            }
        }
    }
    return {};
}

AudioRoute findRoute(const AudioNode* node)
{
    std::unordered_set<const Instance*> visited;
    return findRoute(node, 1.0f, {}, 0, visited);
}

} // namespace

const char* const sAudioPlayer = "AudioPlayer";

static Reflection::PropDescriptor<AudioPlayer, ContentId> propAudioPlayerAsset(
    "Asset", category_Data, &AudioPlayer::getAsset, &AudioPlayer::setAsset);
static Reflection::PropDescriptor<AudioPlayer, std::string> propAudioPlayerAssetId(
    "AssetId", category_Data, &AudioPlayer::getAssetId,
    &AudioPlayer::setAssetId,
    Reflection::PropertyDescriptor::Attributes::deprecated(
        Reflection::PropertyDescriptor::LEGACY_SCRIPTING));
static Reflection::PropDescriptor<AudioPlayer, Content> propAudioPlayerAudioContent(
    "AudioContent", category_Data, &AudioPlayer::getAudioContent,
    &AudioPlayer::setAudioContent);
static Reflection::PropDescriptor<AudioPlayer, bool> propAudioPlayerAutoLoad(
    "AutoLoad", category_Behavior, &AudioPlayer::getAutoLoad,
    &AudioPlayer::setAutoLoad);
static Reflection::PropDescriptor<AudioPlayer, bool> propAudioPlayerAutoPlay(
    "AutoPlay", category_Behavior, &AudioPlayer::getAutoPlay,
    &AudioPlayer::setAutoPlay);
static Reflection::PropDescriptor<AudioPlayer, bool> propAudioPlayerLooping(
    "Looping", category_Behavior, &AudioPlayer::getLooping,
    &AudioPlayer::setLooping);
static Reflection::PropDescriptor<AudioPlayer, NumberRange> propAudioPlayerLoopRegion(
    "LoopRegion", category_Data, &AudioPlayer::getLoopRegion,
    &AudioPlayer::setLoopRegion);
static Reflection::PropDescriptor<AudioPlayer, NumberRange>
    propAudioPlayerPlaybackRegion("PlaybackRegion", category_Data,
        &AudioPlayer::getPlaybackRegion, &AudioPlayer::setPlaybackRegion);
static Reflection::PropDescriptor<AudioPlayer, double> propAudioPlayerPlaybackSpeed(
    "PlaybackSpeed", category_Data, &AudioPlayer::getPlaybackSpeed,
    &AudioPlayer::setPlaybackSpeed);
static Reflection::PropDescriptor<AudioPlayer, float> propAudioPlayerVolume(
    "Volume", category_Data, &AudioPlayer::getVolume,
    &AudioPlayer::setVolume);
static Reflection::PropDescriptor<AudioPlayer, double> propAudioPlayerTimePosition(
    "TimePosition", category_Data, &AudioPlayer::getTimePosition,
    &AudioPlayer::setTimePosition);
static Reflection::PropDescriptor<AudioPlayer, double> propAudioPlayerTimeLength(
    "TimeLength", category_Data, &AudioPlayer::getTimeLength, NULL,
    Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<AudioPlayer, bool> propAudioPlayerIsReady(
    "IsReady", category_Data, &AudioPlayer::getIsReady, NULL,
    Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<AudioPlayer, bool> propAudioPlayerIsPlaying(
    "IsPlaying", category_Data, &AudioPlayer::getIsPlaying, NULL,
    Reflection::PropertyDescriptor::UI);
static Reflection::CustomBoundFuncDesc<AudioPlayer, long long(double)>
    funcAudioPlayerPlay(&AudioPlayer::playLua, "Play", "atTime", -1.0,
        Security::None);
static Reflection::CustomBoundFuncDesc<AudioPlayer, long long(double)>
    funcAudioPlayerStop(&AudioPlayer::stopLua, "Stop", "atTime", -1.0,
        Security::None);
static Reflection::CustomBoundFuncDesc<AudioPlayer, bool(long long)>
    funcAudioPlayerCancel(&AudioPlayer::cancelLua, "Cancel", "actionId",
        static_cast<long long>(0), Security::None);
static Reflection::BoundFuncDesc<AudioPlayer,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioPlayerConnectedWires(&AudioPlayer::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioPlayer,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioPlayerInputPins(&AudioPlayer::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioPlayer,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioPlayerOutputPins(&AudioPlayer::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::EventDesc<AudioPlayer, void()> eventAudioPlayerEnded(
    &AudioPlayer::endedSignal, "Ended", Security::None);
static Reflection::EventDesc<AudioPlayer, void()> eventAudioPlayerLooped(
    &AudioPlayer::loopedSignal, "Looped", Security::None);
static Reflection::EventDesc<AudioPlayer,
    void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> eventAudioPlayerWiringChanged(
    &AudioPlayer::wiringChangedSignal, "WiringChanged", "connected", "pin",
    "wire", "instance", Security::None);

AudioPlayer::AudioPlayer()
    : Super(sAudioPlayer)
    , autoLoad(true)
    , autoPlay(false)
    , looping(false)
    , requestedPlaying(false)
    , loopRegion(0.0f, 60000.0f)
    , playbackRegion(0.0f, 60000.0f)
    , playbackSpeed(1.0)
    , volume(1.0f)
    , stoppedTimePosition(0.0)
    , previousFramePosition(0)
    , emitterAttenuationVersion(0)
    , audioEngine(nullptr)
    , nextScheduledActionId(1)
{
}

AudioPlayer::~AudioPlayer()
{
    releaseVoice();
}

ContentId AudioPlayer::getAsset() const { return asset; }
std::string AudioPlayer::getAssetId() const { return asset.toString(); }
Content AudioPlayer::getAudioContent() const { return audioContent; }
bool AudioPlayer::getAutoLoad() const { return autoLoad; }
bool AudioPlayer::getAutoPlay() const { return autoPlay; }
bool AudioPlayer::getLooping() const { return looping; }
NumberRange AudioPlayer::getLoopRegion() const { return loopRegion; }
NumberRange AudioPlayer::getPlaybackRegion() const { return playbackRegion; }
double AudioPlayer::getPlaybackSpeed() const { return playbackSpeed; }
float AudioPlayer::getVolume() const { return volume; }

void AudioPlayer::setAsset(ContentId value)
{
    if (asset == value)
        return;
    releaseVoice();
    scheduledActions.clear();
    sound.reset();
    asset = value;
    stoppedTimePosition = 0.0;
    requestedPlaying = false;
    raisePropertyChanged(propAudioPlayerAsset);
    raisePropertyChanged(propAudioPlayerAssetId);
    raisePropertyChanged(propAudioPlayerIsReady);
    raisePropertyChanged(propAudioPlayerTimeLength);
    if (autoLoad)
        ensureLoaded();
}

void AudioPlayer::setAssetId(const std::string& value)
{
    setAsset(ContentId(value));
}

void AudioPlayer::setAudioContent(Content value)
{
    if (audioContent == value)
        return;
    releaseVoice();
    scheduledActions.clear();
    sound.reset();
    audioContent = std::move(value);
    stoppedTimePosition = 0.0;
    requestedPlaying = false;
    raisePropertyChanged(propAudioPlayerAudioContent);
    raisePropertyChanged(propAudioPlayerIsReady);
    raisePropertyChanged(propAudioPlayerTimeLength);
    if (autoLoad)
        ensureLoaded();
}

void AudioPlayer::setAutoLoad(bool value)
{
    if (autoLoad == value)
        return;
    autoLoad = value;
    raisePropertyChanged(propAudioPlayerAutoLoad);
    if (autoLoad)
        ensureLoaded();
}

void AudioPlayer::setAutoPlay(bool value)
{
    if (autoPlay == value)
        return;
    autoPlay = value;
    raisePropertyChanged(propAudioPlayerAutoPlay);
    if (autoPlay && ServiceProvider::find<SoundService>(this))
        play();
}

void AudioPlayer::setLooping(bool value)
{
    if (looping == value)
        return;
    looping = value;
    if (voice && audioEngine)
        audioEngine->setVoiceLooping(voice, looping);
    raisePropertyChanged(propAudioPlayerLooping);
}

void AudioPlayer::setLoopRegion(NumberRange value)
{
    if (loopRegion == value)
        return;
    loopRegion = value;
    raisePropertyChanged(propAudioPlayerLoopRegion);
    restartForRegionChange();
}

void AudioPlayer::setPlaybackRegion(NumberRange value)
{
    if (playbackRegion == value)
        return;
    playbackRegion = value;
    raisePropertyChanged(propAudioPlayerPlaybackRegion);
    stoppedTimePosition = std::clamp(stoppedTimePosition,
        static_cast<double>(playbackRegion.min),
        static_cast<double>(playbackRegion.max));
    restartForRegionChange();
}

void AudioPlayer::restartForRegionChange()
{
    if (!voice)
        return;
    double scheduledStart = -1.0;
    for (const ScheduledAction& action : scheduledActions)
        if (action.type == ScheduledActionType::Play)
            scheduledStart = action.mixerTime;
    const bool shouldResume = requestedPlaying && getIsPlaying();
    stoppedTimePosition = getTimePosition();
    releaseVoice();
    if (scheduledStart >= 0.0)
        beginPlayback(scheduledStart);
    else if (shouldResume)
        play();
}

void AudioPlayer::setPlaybackSpeed(double value)
{
    if (!std::isfinite(value))
        throw std::runtime_error("AudioPlayer.PlaybackSpeed must be finite");
    value = std::clamp(value, 0.0, 20.0);
    if (playbackSpeed == value)
        return;
    playbackSpeed = value;
    if (voice && audioEngine)
        audioEngine->setVoicePitch(voice,
            static_cast<float>(std::max(playbackSpeed, 0.01)));
    raisePropertyChanged(propAudioPlayerPlaybackSpeed);
}

void AudioPlayer::setVolume(float value)
{
    if (!std::isfinite(value))
        throw std::runtime_error("AudioPlayer.Volume must be finite");
    value = std::clamp(value, 0.0f, 10.0f);
    if (volume == value)
        return;
    volume = value;
    if (voice && audioEngine)
        audioEngine->setVoiceVolume(voice,
            hasOutputRoute() ? volume * routedVolume() : 0.0f);
    raisePropertyChanged(propAudioPlayerVolume);
}

double AudioPlayer::getTimePosition() const
{
    if (!voice || !audioEngine || !sound)
        return stoppedTimePosition;
    const std::uint32_t rate = audioEngine->clipSampleRate(sound->get());
    return rate ? static_cast<double>(audioEngine->positionFrames(voice)) / rate
                : stoppedTimePosition;
}

void AudioPlayer::setTimePosition(double value)
{
    value = std::max(value, 0.0);
    stoppedTimePosition = value;
    if (voice && audioEngine && sound)
    {
        const std::uint32_t rate = audioEngine->clipSampleRate(sound->get());
        if (rate)
            audioEngine->seekFrames(voice,
                static_cast<std::uint64_t>(value * rate));
    }
    raisePropertyChanged(propAudioPlayerTimePosition);
}

double AudioPlayer::getTimeLength() const
{
    return sound ? sound->getLengthSeconds() : 0.0;
}

bool AudioPlayer::getIsReady() const
{
    return sound && static_cast<bool>(sound->get());
}

bool AudioPlayer::getIsPlaying() const
{
    return voice && audioEngine && audioEngine->isPlaying(voice);
}

bool AudioPlayer::ensureLoaded()
{
    const ContentId selected = selectedAsset();
    if (selected.isNull())
        return false;
    SoundService* service = ServiceProvider::find<SoundService>(this);
    if (!service || !service->enabled())
        return false;
    if (!sound || sound->id != SoundId(selected))
        sound = service->loadSound(SoundId(selected), routedEmitter() != nullptr);
    if (!sound || !sound->tryLoad(this))
        return false;
    audioEngine = &service->getAudioEngine();
    raisePropertyChanged(propAudioPlayerIsReady);
    raisePropertyChanged(propAudioPlayerTimeLength);
    return true;
}

ContentId AudioPlayer::selectedAsset() const
{
    return audioContent.getSourceType() == CONTENT_SOURCE_URI
        ? ContentId(audioContent.getUri()) : asset;
}

void AudioPlayer::play()
{
    requestedPlaying = true;
    if (voice && audioEngine)
    {
        audioEngine->resume(voice);
        raisePropertyChanged(propAudioPlayerIsPlaying);
        return;
    }
    beginPlayback(-1.0);
}

bool AudioPlayer::beginPlayback(double startMixerTime)
{
    if (!ensureLoaded())
        return false;

    AudioEmitter* emitter = routedEmitter();
    Audio::VoiceParameters parameters;
    parameters.volume = hasOutputRoute() ? volume * routedVolume() : 0.0f;
    parameters.pitch = static_cast<float>(std::max(playbackSpeed, 0.01));
    parameters.looping = looping;
    parameters.spatial = emitter != nullptr;
    parameters.listenerIndex = 1;
    parameters.position = emitterPosition(emitter);
    parameters.direction = emitterDirection(emitter);
    parameters.startMixerTimeSeconds = startMixerTime;
    const AudioRoute route = findRoute(this);
    parameters.effects = route.effects;
    parameters.effectCount = route.effectCount;
    const std::uint32_t rate = audioEngine->clipSampleRate(sound->get());
    if (rate)
    {
        const auto secondsToFrames = [rate](float seconds) {
            return static_cast<std::uint64_t>(
                std::max(seconds, 0.0f) * static_cast<double>(rate));
        };
        parameters.rangeBeginFrame = secondsToFrames(playbackRegion.min);
        parameters.rangeEndFrame = secondsToFrames(playbackRegion.max);
        parameters.loopBeginFrame = secondsToFrames(loopRegion.min);
        parameters.loopEndFrame = secondsToFrames(loopRegion.max);
    }
    voice = audioEngine->play(sound->get(), parameters);
    if (!voice)
        return false;
    sound->acquire();
    if (rate && stoppedTimePosition > 0.0)
        audioEngine->seekFrames(voice,
            static_cast<std::uint64_t>(stoppedTimePosition * rate));
    if (emitter && !emitter->distanceAttenuation().empty())
        audioEngine->setVoiceAttenuationCurve(voice,
            emitter->distanceAttenuation());
    if (emitter && !emitter->angleAttenuation().empty())
        audioEngine->setVoiceAngleAttenuationCurve(voice,
            emitter->angleAttenuation());
    emitterAttenuationVersion = emitter ? emitter->attenuationRevision() : 0;
    previousFramePosition = audioEngine->positionFrames(voice);
    raisePropertyChanged(propAudioPlayerIsPlaying);
    return true;
}

void AudioPlayer::stop()
{
    requestedPlaying = false;
    releaseVoice();
    scheduledActions.clear();
    stoppedTimePosition = 0.0;
    raisePropertyChanged(propAudioPlayerTimePosition);
    raisePropertyChanged(propAudioPlayerIsPlaying);
}

long long AudioPlayer::schedulePlay(double mixerTime)
{
    SoundService* service = ServiceProvider::find<SoundService>(this);
    if (!service || !std::isfinite(mixerTime) ||
        mixerTime <= service->getMixerTime())
    {
        play();
        return 0;
    }
    requestedPlaying = true;
    releaseVoice();
    scheduledActions.clear();
    if (!beginPlayback(mixerTime))
        return 0;
    const long long id = nextScheduledActionId++;
    scheduledActions.push_back({id, ScheduledActionType::Play, mixerTime});
    return id;
}

long long AudioPlayer::scheduleStop(double mixerTime)
{
    if (!voice || !audioEngine || !std::isfinite(mixerTime) ||
        mixerTime <= audioEngine->mixerTimeSeconds())
    {
        stop();
        return 0;
    }
    scheduledActions.erase(std::remove_if(scheduledActions.begin(),
        scheduledActions.end(), [](const ScheduledAction& action) {
            return action.type == ScheduledActionType::Stop;
        }), scheduledActions.end());
    if (!audioEngine->scheduleVoiceStop(voice, mixerTime))
        return 0;
    const long long id = nextScheduledActionId++;
    scheduledActions.push_back({id, ScheduledActionType::Stop, mixerTime});
    return id;
}

bool AudioPlayer::cancelScheduledAction(long long actionId)
{
    const auto iterator = std::find_if(scheduledActions.begin(),
        scheduledActions.end(), [actionId](const ScheduledAction& action) {
            return action.id == actionId;
        });
    if (iterator == scheduledActions.end() || !audioEngine ||
        iterator->mixerTime <= audioEngine->mixerTimeSeconds())
        return false;
    if (iterator->type == ScheduledActionType::Play)
    {
        requestedPlaying = false;
        releaseVoice();
    }
    else if (!audioEngine->cancelVoiceStop(voice))
        return false;
    scheduledActions.erase(iterator);
    return true;
}

int AudioPlayer::playLua(lua_State* state)
{
    if (lua_gettop(state) < 2 || lua_isnil(state, 2))
    {
        play();
        return 0;
    }
    if (!lua_isnumber(state, 2))
        throw std::runtime_error("AudioPlayer:Play atTime must be a number or nil");
    const long long actionId = schedulePlay(lua_tonumber(state, 2));
    if (actionId == 0)
        return 0;
    lua_pushnumber(state, static_cast<lua_Number>(actionId));
    return 1;
}

int AudioPlayer::stopLua(lua_State* state)
{
    if (lua_gettop(state) < 2 || lua_isnil(state, 2))
    {
        stop();
        return 0;
    }
    if (!lua_isnumber(state, 2))
        throw std::runtime_error("AudioPlayer:Stop atTime must be a number or nil");
    const long long actionId = scheduleStop(lua_tonumber(state, 2));
    if (actionId == 0)
        return 0;
    lua_pushnumber(state, static_cast<lua_Number>(actionId));
    return 1;
}

int AudioPlayer::cancelLua(lua_State* state)
{
    if (lua_gettop(state) < 2 || lua_isnil(state, 2))
    {
        lua_pushboolean(state, false);
        return 1;
    }
    if (!lua_isnumber(state, 2))
        throw std::runtime_error("AudioPlayer:Cancel actionId must be a number or nil");
    lua_pushboolean(state, cancelScheduledAction(
        static_cast<long long>(lua_tonumber(state, 2))));
    return 1;
}

void AudioPlayer::releaseVoice()
{
    if (voice && audioEngine)
    {
        audioEngine->stop(voice);
        audioEngine->destroyVoice(voice);
        if (sound)
            sound->unacquire();
    }
    voice = {};
    audioEngine = nullptr;
    previousFramePosition = 0;
    emitterAttenuationVersion = 0;
}

void AudioPlayer::update()
{
    if (!voice || !audioEngine)
        return;
    const double mixerTime = audioEngine->mixerTimeSeconds();
    bool committedStop = false;
    scheduledActions.erase(std::remove_if(scheduledActions.begin(),
        scheduledActions.end(), [&](const ScheduledAction& action) {
            if (action.mixerTime > mixerTime)
                return false;
            committedStop |= action.type == ScheduledActionType::Stop;
            return true;
        }), scheduledActions.end());
    if (committedStop)
    {
        requestedPlaying = false;
        stoppedTimePosition = getTimePosition();
        releaseVoice();
        raisePropertyChanged(propAudioPlayerIsPlaying);
        return;
    }
    if (AudioEmitter* emitter = routedEmitter())
    {
        audioEngine->setVoiceTransform(voice, emitterPosition(emitter), {});
        audioEngine->setVoiceDirection(voice, emitterDirection(emitter));
        if (emitterAttenuationVersion != emitter->attenuationRevision())
        {
            audioEngine->setVoiceAttenuationCurve(voice,
                emitter->distanceAttenuation());
            audioEngine->setVoiceAngleAttenuationCurve(voice,
                emitter->angleAttenuation());
            emitterAttenuationVersion = emitter->attenuationRevision();
        }
    }
    audioEngine->setVoiceVolume(voice,
        hasOutputRoute() ? volume * routedVolume() : 0.0f);
    const AudioRoute route = findRoute(this);
    audioEngine->setVoiceEffects(voice, std::span<const Audio::VoiceEffect>(
        route.effects.data(), route.effectCount));
    const std::uint64_t position = audioEngine->positionFrames(voice);
    if (looping && position < previousFramePosition)
        loopedSignal();
    previousFramePosition = position;
    if (!looping && audioEngine->isFinished(voice))
    {
        releaseVoice();
        requestedPlaying = false;
        stoppedTimePosition = 0.0;
        raisePropertyChanged(propAudioPlayerIsPlaying);
        endedSignal();
    }
}

AudioEmitter* AudioPlayer::routedEmitter() const
{
    return findRoute(this).emitter;
}

bool AudioPlayer::hasOutputRoute() const
{
    return findRoute(this).connected;
}

float AudioPlayer::routedVolume() const
{
    const AudioRoute route = findRoute(this);
    return route.connected ? route.gain : 0.0f;
}

Audio::Vector3 AudioPlayer::emitterPosition(AudioEmitter* emitter) const
{
    if (!emitter)
        return {};
    Instance* source = emitter->getPositionType() == EMITTER_POSITION_INSTANCE
        ? emitter->getPositionInstance() : emitter->getParent();
    PartInstance* part = Instance::fastDynamicCast<PartInstance>(source);
    if (!part && source)
        part = Instance::fastDynamicCast<PartInstance>(
            source->findFirstChildByNameRecursive("HumanoidRootPart"));
    if (!part)
        return {};
    const RBX::Vector3 position = part->getCoordinateFrame().translation;
    return {position.x, position.y, position.z};
}

Audio::Vector3 AudioPlayer::emitterDirection(AudioEmitter* emitter) const
{
    if (!emitter)
        return {0.0f, 0.0f, -1.0f};
    Instance* source = emitter->getPositionType() == EMITTER_POSITION_INSTANCE
        ? emitter->getPositionInstance() : emitter->getParent();
    PartInstance* part = Instance::fastDynamicCast<PartInstance>(source);
    if (!part && source)
        part = Instance::fastDynamicCast<PartInstance>(
            source->findFirstChildByNameRecursive("HumanoidRootPart"));
    if (!part)
        return {0.0f, 0.0f, -1.0f};
    const RBX::Vector3 direction = part->getCoordinateFrame().lookVector();
    return {direction.x, direction.y, direction.z};
}

boost::shared_ptr<const Instances>
AudioPlayer::getConnectedWiresReflection(std::string pin)
{
    return getConnectedWires(pin);
}

boost::shared_ptr<const Reflection::ValueArray>
AudioPlayer::getInputPinsReflection()
{
    return getInputPins();
}

boost::shared_ptr<const Reflection::ValueArray>
AudioPlayer::getOutputPinsReflection()
{
    return getOutputPins();
}

std::vector<std::string> AudioPlayer::inputPins() const { return {}; }
std::vector<std::string> AudioPlayer::outputPins() const { return {"Output"}; }

void AudioPlayer::fireWiringChanged(bool connected, const std::string& pin,
    const boost::shared_ptr<Instance>& wireValue,
    const boost::shared_ptr<Instance>& instance)
{
    wiringChangedSignal(connected, pin, wireValue, instance);
    if (voice && audioEngine)
        restartForRegionChange();
}

void AudioPlayer::onServiceProvider(ServiceProvider* oldProvider,
    ServiceProvider* newProvider)
{
    if (SoundService* oldService = ServiceProvider::find<SoundService>(oldProvider))
        oldService->unregisterAudioPlayer(this);
    Super::onServiceProvider(oldProvider, newProvider);
    if (SoundService* newService = ServiceProvider::find<SoundService>(newProvider))
    {
        newService->registerAudioPlayer(this);
        if (autoLoad)
            ensureLoaded();
        if (autoPlay)
            play();
    }
    else
    {
        releaseVoice();
        sound.reset();
    }
}

} // namespace RBX::Soundscape
