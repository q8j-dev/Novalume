#pragma once

#include "V8Tree/Instance.h"
#include "Reflection/Event.h"
#include "Util/BinaryString.h"
#include "Util/Content.h"
#include "Util/ContentId.h"
#include "v8datamodel/NumberRange.h"
#include "audio/AudioEngine.h"

struct lua_State;

namespace RBX {
namespace Network { class Player; }
namespace Soundscape {

class Wire;
class Sound;
class SoundService;

enum EmitterPositionType
{
    EMITTER_POSITION_PARENT = 0,
    EMITTER_POSITION_INSTANCE = 1,
};

enum ListenerPositionType
{
    LISTENER_POSITION_PARENT = 0,
    LISTENER_POSITION_INSTANCE = 1,
};

enum AudioSimulationFidelity
{
    AUDIO_SIMULATION_NONE = 0,
    AUDIO_SIMULATION_AUTOMATIC = 1,
};

enum SimulationMode
{
    SIMULATION_DEFAULT = 0,
    SIMULATION_ENABLED = 1,
    SIMULATION_DISABLED = 2,
};

enum AudioChannelLayout
{
    AUDIO_CHANNEL_MONO = 0,
    AUDIO_CHANNEL_STEREO = 1,
    AUDIO_CHANNEL_QUAD = 2,
    AUDIO_CHANNEL_SURROUND_5 = 3,
    AUDIO_CHANNEL_SURROUND_5_1 = 4,
    AUDIO_CHANNEL_SURROUND_7_1 = 5,
    AUDIO_CHANNEL_SURROUND_7_1_4 = 6,
};

class AudioNode
{
public:
    virtual ~AudioNode() = default;
    virtual std::vector<std::string> inputPins() const = 0;
    virtual std::vector<std::string> outputPins() const = 0;
    virtual Instance* audioNodeInstance() = 0;
    virtual const Instance* audioNodeInstance() const = 0;
    virtual void fireWiringChanged(bool connected, const std::string& pin,
        const boost::shared_ptr<Instance>& wire,
        const boost::shared_ptr<Instance>& instance) = 0;

    boost::shared_ptr<const Instances> getConnectedWires(
        const std::string& pin) const;
    boost::shared_ptr<const Reflection::ValueArray> getInputPins() const;
    boost::shared_ptr<const Reflection::ValueArray> getOutputPins() const;
    bool hasInputPin(const std::string& name) const;
    bool hasOutputPin(const std::string& name) const;
};

extern const char* const sWire;
class Wire final : public DescribedCreatable<Wire, Instance, sWire>
{
public:
    typedef DescribedCreatable<Wire, Instance, sWire> Super;

    Wire();

    Instance* getSourceInstance() const;
    void setSourceInstance(Instance* value);
    const std::string& getSourceName() const;
    void setSourceName(const std::string& value);
    Instance* getTargetInstance() const;
    void setTargetInstance(Instance* value);
    const std::string& getTargetName() const;
    void setTargetName(const std::string& value);
    bool getConnected() const;
    void renameToDefault();

private:
    void notifyEndpoint(AudioNode* endpoint, bool connected,
        const std::string& pin, Instance* other);
    void notifyEndpoints(bool connected);
    void onAncestorChanged(const AncestorChanged& event) override;
    boost::weak_ptr<Instance> sourceInstance;
    boost::weak_ptr<Instance> targetInstance;
    std::string sourceName;
    std::string targetName;
};

extern const char* const sAudioDeviceOutput;
class AudioDeviceOutput final
    : public DescribedCreatable<AudioDeviceOutput, Instance, sAudioDeviceOutput>
    , public AudioNode
{
public:
    AudioDeviceOutput();

    Network::Player* getPlayer() const;
    void setPlayer(Network::Player* value);
    boost::shared_ptr<const Instances> getConnectedWiresReflection(
        std::string pin);
    boost::shared_ptr<const Reflection::ValueArray> getInputPinsReflection();
    boost::shared_ptr<const Reflection::ValueArray> getOutputPinsReflection();

    std::vector<std::string> inputPins() const override;
    std::vector<std::string> outputPins() const override;
    Instance* audioNodeInstance() override { return this; }
    const Instance* audioNodeInstance() const override { return this; }
    void fireWiringChanged(bool connected, const std::string& pin,
        const boost::shared_ptr<Instance>& wire,
        const boost::shared_ptr<Instance>& instance) override
    {
        wiringChangedSignal(connected, pin, wire, instance);
    }

    rbx::signal<void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> wiringChangedSignal;

private:
    boost::weak_ptr<Network::Player> player;
};

extern const char* const sAudioFader;
class AudioFader final
    : public DescribedCreatable<AudioFader, Instance, sAudioFader>
    , public AudioNode
{
public:
    AudioFader();

    bool getBypass() const;
    void setBypass(bool value);
    float getVolume() const;
    void setVolume(float value);
    boost::shared_ptr<const Instances> getConnectedWiresReflection(
        std::string pin);
    boost::shared_ptr<const Reflection::ValueArray> getInputPinsReflection();
    boost::shared_ptr<const Reflection::ValueArray> getOutputPinsReflection();

    std::vector<std::string> inputPins() const override;
    std::vector<std::string> outputPins() const override;
    Instance* audioNodeInstance() override { return this; }
    const Instance* audioNodeInstance() const override { return this; }
    void fireWiringChanged(bool connected, const std::string& pin,
        const boost::shared_ptr<Instance>& wire,
        const boost::shared_ptr<Instance>& instance) override
    {
        wiringChangedSignal(connected, pin, wire, instance);
    }

    rbx::signal<void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> wiringChangedSignal;

private:
    bool bypass;
    float volume;
};

extern const char* const sAudioDistortion;
class AudioDistortion final
    : public DescribedCreatable<AudioDistortion, Instance, sAudioDistortion>
    , public AudioNode
{
public:
    AudioDistortion();

    bool getBypass() const;
    void setBypass(bool value);
    float getLevel() const;
    void setLevel(float value);
    boost::shared_ptr<const Instances> getConnectedWiresReflection(std::string pin);
    boost::shared_ptr<const Reflection::ValueArray> getInputPinsReflection();
    boost::shared_ptr<const Reflection::ValueArray> getOutputPinsReflection();

    std::vector<std::string> inputPins() const override;
    std::vector<std::string> outputPins() const override;
    Instance* audioNodeInstance() override { return this; }
    const Instance* audioNodeInstance() const override { return this; }
    void fireWiringChanged(bool connected, const std::string& pin,
        const boost::shared_ptr<Instance>& wire,
        const boost::shared_ptr<Instance>& instance) override
    {
        wiringChangedSignal(connected, pin, wire, instance);
    }

    rbx::signal<void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> wiringChangedSignal;

private:
    bool bypass;
    float level;
};

extern const char* const sAudioChannelMixer;
class AudioChannelMixer final
    : public DescribedCreatable<AudioChannelMixer, Instance, sAudioChannelMixer>
    , public AudioNode
{
public:
    AudioChannelMixer();
    AudioChannelLayout getLayout() const;
    void setLayout(AudioChannelLayout value);
    boost::shared_ptr<const Instances> getConnectedWiresReflection(std::string pin);
    boost::shared_ptr<const Reflection::ValueArray> getInputPinsReflection();
    boost::shared_ptr<const Reflection::ValueArray> getOutputPinsReflection();
    std::vector<std::string> inputPins() const override;
    std::vector<std::string> outputPins() const override;
    Instance* audioNodeInstance() override { return this; }
    const Instance* audioNodeInstance() const override { return this; }
    void fireWiringChanged(bool connected, const std::string& pin,
        const boost::shared_ptr<Instance>& wire,
        const boost::shared_ptr<Instance>& instance) override
    { wiringChangedSignal(connected, pin, wire, instance); }
    rbx::signal<void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> wiringChangedSignal;
private:
    AudioChannelLayout layout;
};

extern const char* const sAudioChannelSplitter;
class AudioChannelSplitter final
    : public DescribedCreatable<AudioChannelSplitter, Instance, sAudioChannelSplitter>
    , public AudioNode
{
public:
    AudioChannelSplitter();
    AudioChannelLayout getLayout() const;
    void setLayout(AudioChannelLayout value);
    boost::shared_ptr<const Instances> getConnectedWiresReflection(std::string pin);
    boost::shared_ptr<const Reflection::ValueArray> getInputPinsReflection();
    boost::shared_ptr<const Reflection::ValueArray> getOutputPinsReflection();
    std::vector<std::string> inputPins() const override;
    std::vector<std::string> outputPins() const override;
    Instance* audioNodeInstance() override { return this; }
    const Instance* audioNodeInstance() const override { return this; }
    void fireWiringChanged(bool connected, const std::string& pin,
        const boost::shared_ptr<Instance>& wire,
        const boost::shared_ptr<Instance>& instance) override
    { wiringChangedSignal(connected, pin, wire, instance); }
    rbx::signal<void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> wiringChangedSignal;
private:
    AudioChannelLayout layout;
};

extern const char* const sAudioEmitter;
class AudioEmitter final
    : public DescribedCreatable<AudioEmitter, Instance, sAudioEmitter>
    , public AudioNode
{
public:
    AudioEmitter();

    bool getAcousticSimulationEnabled() const;
    void setAcousticSimulationEnabled(bool value);
    const std::string& getAudioInteractionGroup() const;
    void setAudioInteractionGroup(const std::string& value);
    Instance* getPositionInstance() const;
    void setPositionInstance(Instance* value);
    EmitterPositionType getPositionType() const;
    void setPositionType(EmitterPositionType value);
    SimulationMode getDiffractionEnabled() const;
    void setDiffractionEnabled(SimulationMode value);
    SimulationMode getOcclusionEnabled() const;
    void setOcclusionEnabled(SimulationMode value);
    SimulationMode getReverbEnabled() const;
    void setReverbEnabled(SimulationMode value);
    AudioSimulationFidelity getSimulationFidelity() const;
    void setSimulationFidelity(AudioSimulationFidelity value);
    BinaryString getDistanceAttenuationData() const;
    void setDistanceAttenuationData(const BinaryString& value);
    BinaryString getAngleAttenuationData() const;
    void setAngleAttenuationData(const BinaryString& value);
    int getDistanceAttenuationLua(lua_State* state);
    int setDistanceAttenuationLua(lua_State* state);
    int getAngleAttenuationLua(lua_State* state);
    int setAngleAttenuationLua(lua_State* state);
    const std::vector<Audio::AttenuationPoint>& distanceAttenuation() const;
    const std::vector<Audio::AttenuationPoint>& angleAttenuation() const;
    std::uint64_t attenuationRevision() const { return attenuationVersion; }
    float getAudibilityFor(boost::shared_ptr<Instance> listener);
    boost::shared_ptr<const Instances> getInteractingListeners();

    boost::shared_ptr<const Instances> getConnectedWiresReflection(
        std::string pin);
    boost::shared_ptr<const Reflection::ValueArray> getInputPinsReflection();
    boost::shared_ptr<const Reflection::ValueArray> getOutputPinsReflection();
    std::vector<std::string> inputPins() const override;
    std::vector<std::string> outputPins() const override;
    Instance* audioNodeInstance() override { return this; }
    const Instance* audioNodeInstance() const override { return this; }
    void fireWiringChanged(bool connected, const std::string& pin,
        const boost::shared_ptr<Instance>& wire,
        const boost::shared_ptr<Instance>& instance) override
    {
        wiringChangedSignal(connected, pin, wire, instance);
    }

    rbx::signal<void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> wiringChangedSignal;

private:
    void encodeDistanceAttenuation();
    bool decodeDistanceAttenuation();
    void encodeAngleAttenuation();
    bool decodeAngleAttenuation();

    bool acousticSimulationEnabled;
    std::string audioInteractionGroup;
    boost::weak_ptr<Instance> positionInstance;
    EmitterPositionType positionType;
    SimulationMode diffractionEnabled;
    SimulationMode occlusionEnabled;
    SimulationMode reverbEnabled;
    AudioSimulationFidelity simulationFidelity;
    BinaryString distanceAttenuationData;
    std::vector<Audio::AttenuationPoint> distanceAttenuationCurve;
    BinaryString angleAttenuationData;
    std::vector<Audio::AttenuationPoint> angleAttenuationCurve;
    std::uint64_t attenuationVersion;
};

extern const char* const sAudioListener;
class AudioListener final
    : public DescribedCreatable<AudioListener, Instance, sAudioListener>
    , public AudioNode
{
public:
    typedef DescribedCreatable<AudioListener, Instance, sAudioListener> Super;
    AudioListener();

    bool getAcousticSimulationEnabled() const;
    void setAcousticSimulationEnabled(bool value);
    const std::string& getAudioInteractionGroup() const;
    void setAudioInteractionGroup(const std::string& value);
    Instance* getPositionInstance() const;
    void setPositionInstance(Instance* value);
    ListenerPositionType getPositionType() const;
    void setPositionType(ListenerPositionType value);
    SimulationMode getDiffractionEnabled() const;
    void setDiffractionEnabled(SimulationMode value);
    SimulationMode getOcclusionEnabled() const;
    void setOcclusionEnabled(SimulationMode value);
    SimulationMode getReverbEnabled() const;
    void setReverbEnabled(SimulationMode value);
    AudioSimulationFidelity getSimulationFidelity() const;
    void setSimulationFidelity(AudioSimulationFidelity value);
    BinaryString getDistanceAttenuationData() const;
    void setDistanceAttenuationData(const BinaryString& value);
    BinaryString getAngleAttenuationData() const;
    void setAngleAttenuationData(const BinaryString& value);
    int getDistanceAttenuationLua(lua_State* state);
    int setDistanceAttenuationLua(lua_State* state);
    int getAngleAttenuationLua(lua_State* state);
    int setAngleAttenuationLua(lua_State* state);
    const std::vector<Audio::AttenuationPoint>& distanceAttenuation() const;
    const std::vector<Audio::AttenuationPoint>& angleAttenuation() const;
    float getAudibilityFor(boost::shared_ptr<Instance> emitter);
    boost::shared_ptr<const Instances> getInteractingEmitters();
    void reset();
    Audio::ListenerState listenerState() const;

    boost::shared_ptr<const Instances> getConnectedWiresReflection(std::string pin);
    boost::shared_ptr<const Reflection::ValueArray> getInputPinsReflection();
    boost::shared_ptr<const Reflection::ValueArray> getOutputPinsReflection();
    std::vector<std::string> inputPins() const override;
    std::vector<std::string> outputPins() const override;
    Instance* audioNodeInstance() override { return this; }
    const Instance* audioNodeInstance() const override { return this; }
    void fireWiringChanged(bool connected, const std::string& pin,
        const boost::shared_ptr<Instance>& wire,
        const boost::shared_ptr<Instance>& instance) override
    { wiringChangedSignal(connected, pin, wire, instance); }
    rbx::signal<void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> wiringChangedSignal;

protected:
    void onServiceProvider(ServiceProvider* oldProvider,
        ServiceProvider* newProvider) override;

private:
    bool acousticSimulationEnabled;
    std::string audioInteractionGroup;
    boost::weak_ptr<Instance> positionInstance;
    ListenerPositionType positionType;
    SimulationMode diffractionEnabled;
    SimulationMode occlusionEnabled;
    SimulationMode reverbEnabled;
    AudioSimulationFidelity simulationFidelity;
    BinaryString distanceAttenuationData;
    std::vector<Audio::AttenuationPoint> distanceAttenuationCurve;
    BinaryString angleAttenuationData;
    std::vector<Audio::AttenuationPoint> angleAttenuationCurve;
};

extern const char* const sAudioPlayer;
class AudioPlayer final
    : public DescribedCreatable<AudioPlayer, Instance, sAudioPlayer>
    , public AudioNode
{
public:
    typedef DescribedCreatable<AudioPlayer, Instance, sAudioPlayer> Super;

    AudioPlayer();
    ~AudioPlayer() override;

    ContentId getAsset() const;
    void setAsset(ContentId value);
    std::string getAssetId() const;
    void setAssetId(const std::string& value);
    Content getAudioContent() const;
    void setAudioContent(Content value);
    bool getAutoLoad() const;
    void setAutoLoad(bool value);
    bool getAutoPlay() const;
    void setAutoPlay(bool value);
    bool getLooping() const;
    void setLooping(bool value);
    NumberRange getLoopRegion() const;
    void setLoopRegion(NumberRange value);
    NumberRange getPlaybackRegion() const;
    void setPlaybackRegion(NumberRange value);
    double getPlaybackSpeed() const;
    void setPlaybackSpeed(double value);
    float getVolume() const;
    void setVolume(float value);
    double getTimePosition() const;
    void setTimePosition(double value);
    double getTimeLength() const;
    bool getIsReady() const;
    bool getIsPlaying() const;
    void play();
    void stop();
    int playLua(lua_State* state);
    int stopLua(lua_State* state);
    int cancelLua(lua_State* state);

    boost::shared_ptr<const Instances> getConnectedWiresReflection(
        std::string pin);
    boost::shared_ptr<const Reflection::ValueArray> getInputPinsReflection();
    boost::shared_ptr<const Reflection::ValueArray> getOutputPinsReflection();
    std::vector<std::string> inputPins() const override;
    std::vector<std::string> outputPins() const override;
    Instance* audioNodeInstance() override { return this; }
    const Instance* audioNodeInstance() const override { return this; }
    void fireWiringChanged(bool connected, const std::string& pin,
        const boost::shared_ptr<Instance>& wire,
        const boost::shared_ptr<Instance>& instance) override;

    rbx::signal<void()> endedSignal;
    rbx::signal<void()> loopedSignal;
    rbx::signal<void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> wiringChangedSignal;

protected:
    void onServiceProvider(ServiceProvider* oldProvider,
        ServiceProvider* newProvider) override;

private:
    friend class SoundService;
    bool ensureLoaded();
    ContentId selectedAsset() const;
    void restartForRegionChange();
    bool beginPlayback(double startMixerTime);
    long long schedulePlay(double mixerTime);
    long long scheduleStop(double mixerTime);
    bool cancelScheduledAction(long long actionId);
    void releaseVoice();
    void update();
    AudioEmitter* routedEmitter() const;
    bool hasOutputRoute() const;
    float routedVolume() const;
    Audio::Vector3 emitterPosition(AudioEmitter* emitter) const;
    Audio::Vector3 emitterDirection(AudioEmitter* emitter) const;

    ContentId asset;
    Content audioContent;
    bool autoLoad;
    bool autoPlay;
    bool looping;
    bool requestedPlaying;
    NumberRange loopRegion;
    NumberRange playbackRegion;
    double playbackSpeed;
    float volume;
    double stoppedTimePosition;
    std::uint64_t previousFramePosition;
    std::uint64_t emitterAttenuationVersion;
    boost::shared_ptr<Sound> sound;
    Audio::Engine* audioEngine;
    Audio::VoiceHandle voice;
    enum class ScheduledActionType { Play, Stop };
    struct ScheduledAction
    {
        long long id;
        ScheduledActionType type;
        double mixerTime;
    };
    std::vector<ScheduledAction> scheduledActions;
    long long nextScheduledActionId;
};

} // namespace Soundscape
} // namespace RBX
