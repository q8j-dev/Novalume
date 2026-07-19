#include "audio/AudioGraph.h"

#include "Network/Player.h"
#include "Reflection/Reflection.h"
#include "V8DataModel/Attachment.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/PVInstance.h"
#include "audio/SoundService.h"
#include "lua/lua.hpp"

#include <algorithm>
#include <bit>
#include <cmath>
#include <cstdint>
#include <limits>

namespace RBX::Soundscape {
namespace {

AudioNode* asAudioNode(Instance* instance)
{
    return dynamic_cast<AudioNode*>(instance);
}

const AudioNode* asAudioNode(const Instance* instance)
{
    return dynamic_cast<const AudioNode*>(instance);
}

boost::shared_ptr<const Reflection::ValueArray> stringsToValues(
    const std::vector<std::string>& strings)
{
    boost::shared_ptr<Reflection::ValueArray> values(new Reflection::ValueArray());
    values->reserve(strings.size());
    for (const std::string& value : strings)
        values->push_back(Reflection::Variant(value));
    return values;
}

BinaryString encodeCurve(const std::vector<Audio::AttenuationPoint>& curve)
{
    std::string bytes;
    bytes.reserve(8 + curve.size() * 8);
    const auto append32 = [&bytes](std::uint32_t value) {
        for (unsigned shift = 0; shift != 32; shift += 8)
            bytes.push_back(static_cast<char>((value >> shift) & 0xff));
    };
    append32(0x31434152);
    append32(static_cast<std::uint32_t>(curve.size()));
    for (const Audio::AttenuationPoint& point : curve)
    {
        append32(std::bit_cast<std::uint32_t>(point.distance));
        append32(std::bit_cast<std::uint32_t>(point.gain));
    }
    return BinaryString(bytes);
}

bool decodeCurve(const BinaryString& data,
    std::vector<Audio::AttenuationPoint>& curve, bool angle)
{
    const std::string& bytes = data.value();
    const auto read32 = [&bytes](std::size_t offset, std::uint32_t& value) {
        if (offset + 4 > bytes.size()) return false;
        value = static_cast<unsigned char>(bytes[offset]) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 8 |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 16 |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3])) << 24;
        return true;
    };
    std::uint32_t magic = 0, count = 0;
    if (!read32(0, magic) || !read32(4, count) || magic != 0x31434152 ||
        count > 400 || bytes.size() != 8 + count * 8)
        return false;
    std::vector<Audio::AttenuationPoint> decoded;
    decoded.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        std::uint32_t keyBits = 0, gainBits = 0;
        if (!read32(8 + index * 8, keyBits) ||
            !read32(12 + index * 8, gainBits)) return false;
        const float key = std::bit_cast<float>(keyBits);
        const float gain = std::bit_cast<float>(gainBits);
        if (!std::isfinite(key) || !std::isfinite(gain) || key < 0.0f ||
            (angle && key > 180.0f) || gain < 0.0f || gain > 1.0f ||
            (!decoded.empty() && decoded.back().distance >= key)) return false;
        decoded.push_back({key, gain});
    }
    curve = std::move(decoded);
    return true;
}

int pushCurve(lua_State* state,
    const std::vector<Audio::AttenuationPoint>& curve)
{
    lua_createtable(state, 0, static_cast<int>(curve.size()));
    for (const Audio::AttenuationPoint& point : curve)
    {
        lua_pushnumber(state, point.distance);
        lua_pushnumber(state, point.gain);
        lua_settable(state, -3);
    }
    return 1;
}

std::vector<Audio::AttenuationPoint> readCurve(lua_State* state, bool angle)
{
    if (lua_gettop(state) != 2 || (!lua_istable(state, 2) && !lua_isnil(state, 2)))
        throw std::runtime_error("attenuation curve must be a table or nil");
    std::vector<Audio::AttenuationPoint> curve;
    if (lua_isnil(state, 2)) return curve;
    lua_pushnil(state);
    while (lua_next(state, 2) != 0)
    {
        if (!lua_isnumber(state, -2) || !lua_isnumber(state, -1))
        {
            lua_pop(state, 2);
            throw std::runtime_error("attenuation keys and values must be numbers");
        }
        const double key = lua_tonumber(state, -2);
        const double gain = lua_tonumber(state, -1);
        lua_pop(state, 1);
        if (!std::isfinite(key) || !std::isfinite(gain) || key < 0.0 ||
            (angle && key > 180.0) || gain < 0.0 || gain > 1.0 ||
            curve.size() == 400)
        {
            lua_pop(state, 1);
            throw std::runtime_error("attenuation curve is outside current bounds");
        }
        curve.push_back({static_cast<float>(key), static_cast<float>(gain)});
    }
    std::sort(curve.begin(), curve.end(), [](const auto& a, const auto& b) {
        return a.distance < b.distance;
    });
    for (std::size_t index = 1; index < curve.size(); ++index)
        if (curve[index - 1].distance >= curve[index].distance)
            throw std::runtime_error("attenuation keys must be unique");
    return curve;
}

float sampleCurve(const std::vector<Audio::AttenuationPoint>& curve, float key)
{
    if (curve.empty()) return 1.0f;
    if (key <= curve.front().distance) return curve.front().gain;
    for (std::size_t index = 1; index < curve.size(); ++index)
    {
        if (key <= curve[index].distance)
        {
            const auto& a = curve[index - 1];
            const auto& b = curve[index];
            return a.gain + (b.gain - a.gain) *
                ((key - a.distance) / (b.distance - a.distance));
        }
    }
    return curve.back().gain;
}

bool worldFrame(Instance* instance, CoordinateFrame& frame)
{
    if (Attachment* attachment = Instance::fastDynamicCast<Attachment>(instance))
    { frame = attachment->getFrameInWorld(); return true; }
    if (Camera* camera = Instance::fastDynamicCast<Camera>(instance))
    { frame = camera->getCameraCoordinateFrame(); return true; }
    if (IHasLocation* located = dynamic_cast<IHasLocation*>(instance))
    { frame = located->getLocation(); return true; }
    return false;
}

float spatialAudibility(const AudioEmitter& emitter, const AudioListener& listener)
{
    if (emitter.getAudioInteractionGroup() != listener.getAudioInteractionGroup())
        return 0.0f;
    CoordinateFrame emitterFrame, listenerFrame;
    Instance* emitterSource = const_cast<Instance*>(
        emitter.getPositionType() == EMITTER_POSITION_INSTANCE
            ? emitter.getPositionInstance() : emitter.getParent());
    Instance* listenerSource = const_cast<Instance*>(
        listener.getPositionType() == LISTENER_POSITION_INSTANCE
            ? listener.getPositionInstance() : listener.getParent());
    if (!worldFrame(emitterSource, emitterFrame) ||
        !worldFrame(listenerSource, listenerFrame)) return 0.0f;
    const Vector3 delta = listenerFrame.translation - emitterFrame.translation;
    const float distance = delta.magnitude();
    float gain = sampleCurve(emitter.distanceAttenuation(), distance) *
        sampleCurve(listener.distanceAttenuation(), distance);
    if (distance > 0.000001f)
    {
        const Vector3 toListener = delta / distance;
        const float emitterAngle = std::acos(std::clamp(
            emitterFrame.lookVector().dot(toListener), -1.0f, 1.0f)) *
            (180.0f / 3.14159265358979323846f);
        const float listenerAngle = std::acos(std::clamp(
            listenerFrame.lookVector().dot(-toListener), -1.0f, 1.0f)) *
            (180.0f / 3.14159265358979323846f);
        gain *= sampleCurve(emitter.angleAttenuation(), emitterAngle) *
            sampleCurve(listener.angleAttenuation(), listenerAngle);
    }
    return std::clamp(gain, 0.0f, 1.0f);
}

} // namespace

} // namespace RBX::Soundscape

namespace RBX::Reflection {

template<> EnumDesc<RBX::Soundscape::EmitterPositionType>::EnumDesc()
    : EnumDescriptor("EmitterPositionType")
{
    addPair(RBX::Soundscape::EMITTER_POSITION_PARENT, "Parent");
    addPair(RBX::Soundscape::EMITTER_POSITION_INSTANCE, "Instance");
}

template<> EnumDesc<RBX::Soundscape::ListenerPositionType>::EnumDesc()
    : EnumDescriptor("ListenerPositionType")
{
    addPair(RBX::Soundscape::LISTENER_POSITION_PARENT, "Parent");
    addPair(RBX::Soundscape::LISTENER_POSITION_INSTANCE, "Instance");
}

template<> EnumDesc<RBX::Soundscape::AudioSimulationFidelity>::EnumDesc()
    : EnumDescriptor("AudioSimulationFidelity")
{
    addPair(RBX::Soundscape::AUDIO_SIMULATION_NONE, "None");
    addPair(RBX::Soundscape::AUDIO_SIMULATION_AUTOMATIC, "Automatic");
}

template<> EnumDesc<RBX::Soundscape::SimulationMode>::EnumDesc()
    : EnumDescriptor("SimulationMode")
{
    addPair(RBX::Soundscape::SIMULATION_DEFAULT, "Default");
    addPair(RBX::Soundscape::SIMULATION_ENABLED, "Enabled");
    addPair(RBX::Soundscape::SIMULATION_DISABLED, "Disabled");
}

template<> EnumDesc<RBX::Soundscape::AudioChannelLayout>::EnumDesc()
    : EnumDescriptor("AudioChannelLayout")
{
    addPair(RBX::Soundscape::AUDIO_CHANNEL_MONO, "Mono");
    addPair(RBX::Soundscape::AUDIO_CHANNEL_STEREO, "Stereo");
    addPair(RBX::Soundscape::AUDIO_CHANNEL_QUAD, "Quad");
    addPair(RBX::Soundscape::AUDIO_CHANNEL_SURROUND_5, "Surround_5");
    addPair(RBX::Soundscape::AUDIO_CHANNEL_SURROUND_5_1, "Surround_5_1");
    addPair(RBX::Soundscape::AUDIO_CHANNEL_SURROUND_7_1, "Surround_7_1");
    addPair(RBX::Soundscape::AUDIO_CHANNEL_SURROUND_7_1_4, "Surround_7_1_4");
}

template<> RBX::Soundscape::EmitterPositionType&
Variant::convert<RBX::Soundscape::EmitterPositionType>()
{
    return genericConvert<RBX::Soundscape::EmitterPositionType>();
}

template<> RBX::Soundscape::ListenerPositionType&
Variant::convert<RBX::Soundscape::ListenerPositionType>()
{ return genericConvert<RBX::Soundscape::ListenerPositionType>(); }

template<> RBX::Soundscape::AudioSimulationFidelity&
Variant::convert<RBX::Soundscape::AudioSimulationFidelity>()
{ return genericConvert<RBX::Soundscape::AudioSimulationFidelity>(); }

template<> RBX::Soundscape::SimulationMode&
Variant::convert<RBX::Soundscape::SimulationMode>()
{ return genericConvert<RBX::Soundscape::SimulationMode>(); }

template<> RBX::Soundscape::AudioChannelLayout&
Variant::convert<RBX::Soundscape::AudioChannelLayout>()
{ return genericConvert<RBX::Soundscape::AudioChannelLayout>(); }

} // namespace RBX::Reflection

namespace RBX {

template<> bool StringConverter<Soundscape::EmitterPositionType>::convertToValue(
    const std::string& text, Soundscape::EmitterPositionType& value)
{
    return Reflection::EnumDesc<Soundscape::EmitterPositionType>::singleton()
        .convertToValue(text.c_str(), value);
}

template<> bool StringConverter<Soundscape::ListenerPositionType>::convertToValue(
    const std::string& text, Soundscape::ListenerPositionType& value)
{ return Reflection::EnumDesc<Soundscape::ListenerPositionType>::singleton().convertToValue(text.c_str(), value); }

template<> bool StringConverter<Soundscape::AudioSimulationFidelity>::convertToValue(
    const std::string& text, Soundscape::AudioSimulationFidelity& value)
{ return Reflection::EnumDesc<Soundscape::AudioSimulationFidelity>::singleton().convertToValue(text.c_str(), value); }

template<> bool StringConverter<Soundscape::SimulationMode>::convertToValue(
    const std::string& text, Soundscape::SimulationMode& value)
{ return Reflection::EnumDesc<Soundscape::SimulationMode>::singleton().convertToValue(text.c_str(), value); }

template<> bool StringConverter<Soundscape::AudioChannelLayout>::convertToValue(
    const std::string& text, Soundscape::AudioChannelLayout& value)
{ return Reflection::EnumDesc<Soundscape::AudioChannelLayout>::singleton().convertToValue(text.c_str(), value); }

} // namespace RBX

namespace RBX::Soundscape {

const char* const sWire = "Wire";
const char* const sAudioDeviceOutput = "AudioDeviceOutput";
const char* const sAudioFader = "AudioFader";
const char* const sAudioDistortion = "AudioDistortion";
const char* const sAudioTremolo = "AudioTremolo";
const char* const sAudioChorus = "AudioChorus";
const char* const sAudioFlanger = "AudioFlanger";
const char* const sAudioCompressor = "AudioCompressor";
const char* const sAudioChannelMixer = "AudioChannelMixer";
const char* const sAudioChannelSplitter = "AudioChannelSplitter";
const char* const sAudioEmitter = "AudioEmitter";
const char* const sAudioListener = "AudioListener";

static Reflection::RefPropDescriptor<Wire, Instance> propWireSourceInstance(
    "SourceInstance", category_Data, &Wire::getSourceInstance,
    &Wire::setSourceInstance, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<Wire, std::string> propWireSourceName(
    "SourceName", category_Data, &Wire::getSourceName, &Wire::setSourceName,
    Reflection::PropertyDescriptor::STANDARD);
static Reflection::RefPropDescriptor<Wire, Instance> propWireTargetInstance(
    "TargetInstance", category_Data, &Wire::getTargetInstance,
    &Wire::setTargetInstance, Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<Wire, std::string> propWireTargetName(
    "TargetName", category_Data, &Wire::getTargetName, &Wire::setTargetName,
    Reflection::PropertyDescriptor::STANDARD);
static Reflection::PropDescriptor<Wire, bool> propWireConnected(
    "Connected", category_Data, &Wire::getConnected, NULL,
    Reflection::PropertyDescriptor::UI);
static Reflection::BoundFuncDesc<Wire, void()> funcWireRenameToDefault(
    &Wire::renameToDefault, "RenameToDefault", Security::RobloxScript);

static Reflection::RefPropDescriptor<AudioDeviceOutput, Network::Player>
    propAudioDeviceOutputPlayer("Player", category_Data,
        &AudioDeviceOutput::getPlayer, &AudioDeviceOutput::setPlayer,
        Reflection::PropertyDescriptor::STANDARD);
static Reflection::BoundFuncDesc<AudioDeviceOutput,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioDeviceOutputConnectedWires(
        &AudioDeviceOutput::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioDeviceOutput,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioDeviceOutputInputPins(&AudioDeviceOutput::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioDeviceOutput,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioDeviceOutputOutputPins(&AudioDeviceOutput::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::EventDesc<AudioDeviceOutput,
    void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> eventAudioDeviceOutputWiringChanged(
    &AudioDeviceOutput::wiringChangedSignal, "WiringChanged", "connected",
    "pin", "wire", "instance", Security::None);

static Reflection::PropDescriptor<AudioFader, bool> propAudioFaderBypass(
    "Bypass", category_Behavior, &AudioFader::getBypass,
    &AudioFader::setBypass);
static Reflection::PropDescriptor<AudioFader, float> propAudioFaderVolume(
    "Volume", category_Data, &AudioFader::getVolume, &AudioFader::setVolume);
static Reflection::BoundFuncDesc<AudioFader,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioFaderConnectedWires(&AudioFader::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioFader,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioFaderInputPins(&AudioFader::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioFader,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioFaderOutputPins(&AudioFader::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::EventDesc<AudioFader,
    void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> eventAudioFaderWiringChanged(
    &AudioFader::wiringChangedSignal, "WiringChanged", "connected", "pin",
    "wire", "instance", Security::None);

static Reflection::PropDescriptor<AudioDistortion, bool>
    propAudioDistortionBypass("Bypass", category_Behavior,
        &AudioDistortion::getBypass, &AudioDistortion::setBypass);
static Reflection::PropDescriptor<AudioDistortion, float>
    propAudioDistortionLevel("Level", category_Data,
        &AudioDistortion::getLevel, &AudioDistortion::setLevel);
static Reflection::BoundFuncDesc<AudioDistortion,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioDistortionConnectedWires(
        &AudioDistortion::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioDistortion,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioDistortionInputPins(&AudioDistortion::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioDistortion,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioDistortionOutputPins(&AudioDistortion::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::EventDesc<AudioDistortion,
    void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> eventAudioDistortionWiringChanged(
    &AudioDistortion::wiringChangedSignal, "WiringChanged", "connected", "pin",
    "wire", "instance", Security::None);

static Reflection::PropDescriptor<AudioTremolo, bool> propAudioTremoloBypass(
    "Bypass", category_Behavior, &AudioTremolo::getBypass,
    &AudioTremolo::setBypass);
static Reflection::PropDescriptor<AudioTremolo, float> propAudioTremoloDepth(
    "Depth", category_Data, &AudioTremolo::getDepth, &AudioTremolo::setDepth);
static Reflection::PropDescriptor<AudioTremolo, float> propAudioTremoloDuty(
    "Duty", category_Data, &AudioTremolo::getDuty, &AudioTremolo::setDuty);
static Reflection::PropDescriptor<AudioTremolo, float> propAudioTremoloFrequency(
    "Frequency", category_Data, &AudioTremolo::getFrequency,
    &AudioTremolo::setFrequency);
static Reflection::PropDescriptor<AudioTremolo, float> propAudioTremoloShape(
    "Shape", category_Data, &AudioTremolo::getShape, &AudioTremolo::setShape);
static Reflection::PropDescriptor<AudioTremolo, float> propAudioTremoloSkew(
    "Skew", category_Data, &AudioTremolo::getSkew, &AudioTremolo::setSkew);
static Reflection::PropDescriptor<AudioTremolo, float> propAudioTremoloSquare(
    "Square", category_Data, &AudioTremolo::getSquare, &AudioTremolo::setSquare);
static Reflection::BoundFuncDesc<AudioTremolo,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioTremoloConnectedWires(&AudioTremolo::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioTremolo,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioTremoloInputPins(&AudioTremolo::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioTremolo,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioTremoloOutputPins(&AudioTremolo::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::EventDesc<AudioTremolo,
    void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> eventAudioTremoloWiringChanged(
    &AudioTremolo::wiringChangedSignal, "WiringChanged", "connected", "pin",
    "wire", "instance", Security::None);

#define RBX_DEFINE_AUDIO_MODULATION_REFLECTION(ClassName, Prefix) \
static Reflection::PropDescriptor<ClassName, bool> Prefix##Bypass( \
    "Bypass", category_Behavior, &ClassName::getBypass, &ClassName::setBypass); \
static Reflection::PropDescriptor<ClassName, float> Prefix##Depth( \
    "Depth", category_Data, &ClassName::getDepth, &ClassName::setDepth); \
static Reflection::PropDescriptor<ClassName, float> Prefix##Mix( \
    "Mix", category_Data, &ClassName::getMix, &ClassName::setMix); \
static Reflection::PropDescriptor<ClassName, float> Prefix##Rate( \
    "Rate", category_Data, &ClassName::getRate, &ClassName::setRate); \
static Reflection::BoundFuncDesc<ClassName, \
    boost::shared_ptr<const Instances>(std::string)> Prefix##ConnectedWires( \
    &ClassName::getConnectedWiresReflection, "GetConnectedWires", "pin", \
    Security::None); \
static Reflection::BoundFuncDesc<ClassName, \
    boost::shared_ptr<const Reflection::ValueArray>()> Prefix##InputPins( \
    &ClassName::getInputPinsReflection, "GetInputPins", Security::None); \
static Reflection::BoundFuncDesc<ClassName, \
    boost::shared_ptr<const Reflection::ValueArray>()> Prefix##OutputPins( \
    &ClassName::getOutputPinsReflection, "GetOutputPins", Security::None); \
static Reflection::EventDesc<ClassName, void(bool, std::string, \
    boost::shared_ptr<Instance>, boost::shared_ptr<Instance>)> \
    Prefix##WiringChanged(&ClassName::wiringChangedSignal, "WiringChanged", \
        "connected", "pin", "wire", "instance", Security::None)

RBX_DEFINE_AUDIO_MODULATION_REFLECTION(AudioChorus, propAudioChorus);
RBX_DEFINE_AUDIO_MODULATION_REFLECTION(AudioFlanger, propAudioFlanger);
#undef RBX_DEFINE_AUDIO_MODULATION_REFLECTION

static Reflection::PropDescriptor<AudioCompressor, float>
    propAudioCompressorAttack("Attack", category_Data,
        &AudioCompressor::getAttack, &AudioCompressor::setAttack);
static Reflection::PropDescriptor<AudioCompressor, bool>
    propAudioCompressorBypass("Bypass", category_Behavior,
        &AudioCompressor::getBypass, &AudioCompressor::setBypass);
static Reflection::PropDescriptor<AudioCompressor, bool>
    propAudioCompressorEditor("Editor", category_Data,
        &AudioCompressor::getEditor, &AudioCompressor::setEditor,
        Reflection::PropertyDescriptor::UI, Security::Roblox);
static Reflection::PropDescriptor<AudioCompressor, float>
    propAudioCompressorMakeupGain("MakeupGain", category_Data,
        &AudioCompressor::getMakeupGain, &AudioCompressor::setMakeupGain);
static Reflection::PropDescriptor<AudioCompressor, float>
    propAudioCompressorRatio("Ratio", category_Data,
        &AudioCompressor::getRatio, &AudioCompressor::setRatio);
static Reflection::PropDescriptor<AudioCompressor, float>
    propAudioCompressorRelease("Release", category_Data,
        &AudioCompressor::getRelease, &AudioCompressor::setRelease);
static Reflection::PropDescriptor<AudioCompressor, float>
    propAudioCompressorThreshold("Threshold", category_Data,
        &AudioCompressor::getThreshold, &AudioCompressor::setThreshold);
static Reflection::BoundFuncDesc<AudioCompressor,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioCompressorConnectedWires(
        &AudioCompressor::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioCompressor,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioCompressorInputPins(&AudioCompressor::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioCompressor,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioCompressorOutputPins(&AudioCompressor::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::EventDesc<AudioCompressor,
    void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> eventAudioCompressorWiringChanged(
    &AudioCompressor::wiringChangedSignal, "WiringChanged", "connected", "pin",
    "wire", "instance", Security::None);

static Reflection::EnumPropDescriptor<AudioChannelMixer, AudioChannelLayout>
    propAudioChannelMixerLayout("Layout", category_Data,
        &AudioChannelMixer::getLayout, &AudioChannelMixer::setLayout);
static Reflection::BoundFuncDesc<AudioChannelMixer,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioChannelMixerConnectedWires(
        &AudioChannelMixer::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioChannelMixer,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioChannelMixerInputPins(&AudioChannelMixer::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioChannelMixer,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioChannelMixerOutputPins(&AudioChannelMixer::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::EventDesc<AudioChannelMixer,
    void(bool, std::string, boost::shared_ptr<Instance>, boost::shared_ptr<Instance>)>
    eventAudioChannelMixerWiringChanged(&AudioChannelMixer::wiringChangedSignal,
        "WiringChanged", "connected", "pin", "wire", "instance", Security::None);

static Reflection::EnumPropDescriptor<AudioChannelSplitter, AudioChannelLayout>
    propAudioChannelSplitterLayout("Layout", category_Data,
        &AudioChannelSplitter::getLayout, &AudioChannelSplitter::setLayout);
static Reflection::BoundFuncDesc<AudioChannelSplitter,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioChannelSplitterConnectedWires(
        &AudioChannelSplitter::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioChannelSplitter,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioChannelSplitterInputPins(&AudioChannelSplitter::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioChannelSplitter,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioChannelSplitterOutputPins(&AudioChannelSplitter::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::EventDesc<AudioChannelSplitter,
    void(bool, std::string, boost::shared_ptr<Instance>, boost::shared_ptr<Instance>)>
    eventAudioChannelSplitterWiringChanged(&AudioChannelSplitter::wiringChangedSignal,
        "WiringChanged", "connected", "pin", "wire", "instance", Security::None);

static Reflection::PropDescriptor<AudioEmitter, bool>
    propAudioEmitterAcousticSimulationEnabled("AcousticSimulationEnabled",
        category_Behavior, &AudioEmitter::getAcousticSimulationEnabled,
        &AudioEmitter::setAcousticSimulationEnabled);
static Reflection::PropDescriptor<AudioEmitter, std::string>
    propAudioEmitterInteractionGroup("AudioInteractionGroup", category_Data,
        &AudioEmitter::getAudioInteractionGroup,
        &AudioEmitter::setAudioInteractionGroup);
static Reflection::RefPropDescriptor<AudioEmitter, Instance>
    propAudioEmitterPositionInstance("PositionInstance", category_Data,
        &AudioEmitter::getPositionInstance, &AudioEmitter::setPositionInstance,
        Reflection::PropertyDescriptor::STANDARD);
static Reflection::EnumPropDescriptor<AudioEmitter, EmitterPositionType>
    propAudioEmitterPositionType("PositionType", category_Data,
        &AudioEmitter::getPositionType, &AudioEmitter::setPositionType);
static Reflection::EnumPropDescriptor<AudioEmitter, SimulationMode>
    propAudioEmitterDiffraction("DiffractionEnabled", category_Behavior,
        &AudioEmitter::getDiffractionEnabled, &AudioEmitter::setDiffractionEnabled);
static Reflection::EnumPropDescriptor<AudioEmitter, SimulationMode>
    propAudioEmitterOcclusion("OcclusionEnabled", category_Behavior,
        &AudioEmitter::getOcclusionEnabled, &AudioEmitter::setOcclusionEnabled);
static Reflection::EnumPropDescriptor<AudioEmitter, SimulationMode>
    propAudioEmitterReverb("ReverbEnabled", category_Behavior,
        &AudioEmitter::getReverbEnabled, &AudioEmitter::setReverbEnabled);
static Reflection::EnumPropDescriptor<AudioEmitter, AudioSimulationFidelity>
    propAudioEmitterSimulationFidelity("SimulationFidelity", category_Behavior,
        &AudioEmitter::getSimulationFidelity, &AudioEmitter::setSimulationFidelity);
static Reflection::PropDescriptor<AudioEmitter, BinaryString>
    propAudioEmitterDistanceAttenuation("DistanceAttenuation", category_Data,
        &AudioEmitter::getDistanceAttenuationData,
        &AudioEmitter::setDistanceAttenuationData,
        Reflection::PropertyDescriptor::STANDARD, Security::Roblox);
static Reflection::PropDescriptor<AudioEmitter, BinaryString>
    propAudioEmitterAngleAttenuation("AngleAttenuation", category_Data,
        &AudioEmitter::getAngleAttenuationData,
        &AudioEmitter::setAngleAttenuationData,
        Reflection::PropertyDescriptor::STANDARD, Security::Roblox);
static Reflection::CustomBoundFuncDesc<AudioEmitter,
    boost::shared_ptr<const Reflection::ValueTable>()>
    funcAudioEmitterGetDistanceAttenuation(
        &AudioEmitter::getDistanceAttenuationLua, "GetDistanceAttenuation",
        Security::None);
static Reflection::CustomBoundFuncDesc<AudioEmitter,
    void(boost::shared_ptr<const Reflection::ValueTable>)>
    funcAudioEmitterSetDistanceAttenuation(
        &AudioEmitter::setDistanceAttenuationLua, "SetDistanceAttenuation",
        "curve", Security::None);
static Reflection::CustomBoundFuncDesc<AudioEmitter,
    boost::shared_ptr<const Reflection::ValueTable>()>
    funcAudioEmitterGetAngleAttenuation(
        &AudioEmitter::getAngleAttenuationLua, "GetAngleAttenuation",
        Security::None);
static Reflection::CustomBoundFuncDesc<AudioEmitter,
    void(boost::shared_ptr<const Reflection::ValueTable>)>
    funcAudioEmitterSetAngleAttenuation(
        &AudioEmitter::setAngleAttenuationLua, "SetAngleAttenuation",
        "curve", Security::None);
static Reflection::BoundFuncDesc<AudioEmitter,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioEmitterConnectedWires(&AudioEmitter::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioEmitter,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioEmitterInputPins(&AudioEmitter::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioEmitter,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioEmitterOutputPins(&AudioEmitter::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::BoundFuncDesc<AudioEmitter, float(boost::shared_ptr<Instance>)>
    funcAudioEmitterAudibility(&AudioEmitter::getAudibilityFor,
        "GetAudibilityFor", "listener", Security::None);
static Reflection::BoundFuncDesc<AudioEmitter, boost::shared_ptr<const Instances>()>
    funcAudioEmitterInteractingListeners(&AudioEmitter::getInteractingListeners,
        "GetInteractingListeners", Security::None);
static Reflection::EventDesc<AudioEmitter,
    void(bool, std::string, boost::shared_ptr<Instance>,
        boost::shared_ptr<Instance>)> eventAudioEmitterWiringChanged(
    &AudioEmitter::wiringChangedSignal, "WiringChanged", "connected", "pin",
    "wire", "instance", Security::None);

static Reflection::PropDescriptor<AudioListener, bool>
    propAudioListenerAcoustic("AcousticSimulationEnabled", category_Behavior,
        &AudioListener::getAcousticSimulationEnabled,
        &AudioListener::setAcousticSimulationEnabled);
static Reflection::PropDescriptor<AudioListener, std::string>
    propAudioListenerInteractionGroup("AudioInteractionGroup", category_Data,
        &AudioListener::getAudioInteractionGroup,
        &AudioListener::setAudioInteractionGroup);
static Reflection::RefPropDescriptor<AudioListener, Instance>
    propAudioListenerPositionInstance("PositionInstance", category_Data,
        &AudioListener::getPositionInstance, &AudioListener::setPositionInstance,
        Reflection::PropertyDescriptor::STANDARD);
static Reflection::EnumPropDescriptor<AudioListener, ListenerPositionType>
    propAudioListenerPositionType("PositionType", category_Data,
        &AudioListener::getPositionType, &AudioListener::setPositionType);
static Reflection::EnumPropDescriptor<AudioListener, SimulationMode>
    propAudioListenerDiffraction("DiffractionEnabled", category_Behavior,
        &AudioListener::getDiffractionEnabled, &AudioListener::setDiffractionEnabled);
static Reflection::EnumPropDescriptor<AudioListener, SimulationMode>
    propAudioListenerOcclusion("OcclusionEnabled", category_Behavior,
        &AudioListener::getOcclusionEnabled, &AudioListener::setOcclusionEnabled);
static Reflection::EnumPropDescriptor<AudioListener, SimulationMode>
    propAudioListenerReverb("ReverbEnabled", category_Behavior,
        &AudioListener::getReverbEnabled, &AudioListener::setReverbEnabled);
static Reflection::EnumPropDescriptor<AudioListener, AudioSimulationFidelity>
    propAudioListenerSimulationFidelity("SimulationFidelity", category_Behavior,
        &AudioListener::getSimulationFidelity, &AudioListener::setSimulationFidelity);
static Reflection::PropDescriptor<AudioListener, BinaryString>
    propAudioListenerDistance("DistanceAttenuation", category_Data,
        &AudioListener::getDistanceAttenuationData,
        &AudioListener::setDistanceAttenuationData,
        Reflection::PropertyDescriptor::STANDARD, Security::Roblox);
static Reflection::PropDescriptor<AudioListener, BinaryString>
    propAudioListenerAngle("AngleAttenuation", category_Data,
        &AudioListener::getAngleAttenuationData,
        &AudioListener::setAngleAttenuationData,
        Reflection::PropertyDescriptor::STANDARD, Security::Roblox);
static Reflection::CustomBoundFuncDesc<AudioListener,
    boost::shared_ptr<const Reflection::ValueTable>()>
    funcAudioListenerGetDistance(&AudioListener::getDistanceAttenuationLua,
        "GetDistanceAttenuation", Security::None);
static Reflection::CustomBoundFuncDesc<AudioListener,
    void(boost::shared_ptr<const Reflection::ValueTable>)>
    funcAudioListenerSetDistance(&AudioListener::setDistanceAttenuationLua,
        "SetDistanceAttenuation", "curve", Security::None);
static Reflection::CustomBoundFuncDesc<AudioListener,
    boost::shared_ptr<const Reflection::ValueTable>()>
    funcAudioListenerGetAngle(&AudioListener::getAngleAttenuationLua,
        "GetAngleAttenuation", Security::None);
static Reflection::CustomBoundFuncDesc<AudioListener,
    void(boost::shared_ptr<const Reflection::ValueTable>)>
    funcAudioListenerSetAngle(&AudioListener::setAngleAttenuationLua,
        "SetAngleAttenuation", "curve", Security::None);
static Reflection::BoundFuncDesc<AudioListener,
    boost::shared_ptr<const Instances>(std::string)>
    funcAudioListenerConnectedWires(&AudioListener::getConnectedWiresReflection,
        "GetConnectedWires", "pin", Security::None);
static Reflection::BoundFuncDesc<AudioListener,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioListenerInputPins(&AudioListener::getInputPinsReflection,
        "GetInputPins", Security::None);
static Reflection::BoundFuncDesc<AudioListener,
    boost::shared_ptr<const Reflection::ValueArray>()>
    funcAudioListenerOutputPins(&AudioListener::getOutputPinsReflection,
        "GetOutputPins", Security::None);
static Reflection::BoundFuncDesc<AudioListener, float(boost::shared_ptr<Instance>)>
    funcAudioListenerAudibility(&AudioListener::getAudibilityFor,
        "GetAudibilityFor", "emitter", Security::None);
static Reflection::BoundFuncDesc<AudioListener, boost::shared_ptr<const Instances>()>
    funcAudioListenerInteractingEmitters(&AudioListener::getInteractingEmitters,
        "GetInteractingEmitters", Security::None);
static Reflection::BoundFuncDesc<AudioListener, void()>
    funcAudioListenerReset(&AudioListener::reset, "Reset", Security::None);
static Reflection::EventDesc<AudioListener,
    void(bool, std::string, boost::shared_ptr<Instance>, boost::shared_ptr<Instance>)>
    eventAudioListenerWiringChanged(&AudioListener::wiringChangedSignal,
        "WiringChanged", "connected", "pin", "wire", "instance", Security::None);

boost::shared_ptr<const Instances> AudioNode::getConnectedWires(
    const std::string& pin) const
{
    boost::shared_ptr<Instances> result(new Instances());
    const Instance* node = audioNodeInstance();
    const Instance* root = node ? node->getRootAncestor() : nullptr;
    if (!root)
        return result;
    root->visitDescendants([&](const boost::shared_ptr<Instance>& descendant) {
        Wire* wire = Instance::fastDynamicCast<Wire>(descendant.get());
        const bool sourceMatch = wire && wire->getSourceInstance() == node &&
            wire->getSourceName() == pin;
        const bool targetMatch = wire && wire->getTargetInstance() == node &&
            wire->getTargetName() == pin;
        if (wire && wire->getConnected() && (sourceMatch || targetMatch))
            result->push_back(descendant);
    });
    return result;
}

boost::shared_ptr<const Reflection::ValueArray> AudioNode::getInputPins() const
{
    return stringsToValues(inputPins());
}

boost::shared_ptr<const Reflection::ValueArray> AudioNode::getOutputPins() const
{
    return stringsToValues(outputPins());
}

bool AudioNode::hasInputPin(const std::string& name) const
{
    const std::vector<std::string> pins = inputPins();
    return std::find(pins.begin(), pins.end(), name) != pins.end();
}

bool AudioNode::hasOutputPin(const std::string& name) const
{
    const std::vector<std::string> pins = outputPins();
    return std::find(pins.begin(), pins.end(), name) != pins.end();
}

Wire::Wire()
    : DescribedCreatable<Wire, Instance, sWire>(sWire)
{
}

Instance* Wire::getSourceInstance() const { return sourceInstance.lock().get(); }
Instance* Wire::getTargetInstance() const { return targetInstance.lock().get(); }
const std::string& Wire::getSourceName() const { return sourceName; }
const std::string& Wire::getTargetName() const { return targetName; }

void Wire::setSourceInstance(Instance* value)
{
    if (getSourceInstance() == value)
        return;
    notifyEndpoints(false);
    sourceInstance = value ? weak_from(value) : boost::weak_ptr<Instance>();
    raisePropertyChanged(propWireSourceInstance);
    if (sourceName.empty())
    {
        const AudioNode* source = asAudioNode(value);
        const std::vector<std::string> pins = source
            ? source->outputPins() : std::vector<std::string>();
        if (!pins.empty())
        {
            sourceName = pins.front();
            raisePropertyChanged(propWireSourceName);
        }
    }
    raisePropertyChanged(propWireConnected);
    notifyEndpoints(getConnected());
}

void Wire::setTargetInstance(Instance* value)
{
    if (getTargetInstance() == value)
        return;
    notifyEndpoints(false);
    targetInstance = value ? weak_from(value) : boost::weak_ptr<Instance>();
    raisePropertyChanged(propWireTargetInstance);
    if (targetName.empty())
    {
        const AudioNode* target = asAudioNode(value);
        const std::vector<std::string> pins = target
            ? target->inputPins() : std::vector<std::string>();
        if (!pins.empty())
        {
            targetName = pins.front();
            raisePropertyChanged(propWireTargetName);
        }
    }
    raisePropertyChanged(propWireConnected);
    notifyEndpoints(getConnected());
}

void Wire::setSourceName(const std::string& value)
{
    if (sourceName == value)
        return;
    notifyEndpoints(false);
    sourceName = value;
    raisePropertyChanged(propWireSourceName);
    raisePropertyChanged(propWireConnected);
    notifyEndpoints(getConnected());
}

void Wire::setTargetName(const std::string& value)
{
    if (targetName == value)
        return;
    notifyEndpoints(false);
    targetName = value;
    raisePropertyChanged(propWireTargetName);
    raisePropertyChanged(propWireConnected);
    notifyEndpoints(getConnected());
}

bool Wire::getConnected() const
{
    const AudioNode* source = asAudioNode(getSourceInstance());
    const AudioNode* target = asAudioNode(getTargetInstance());
    const Instance* parent = getParent();
    const Instance* sourceInstanceValue = source
        ? source->audioNodeInstance() : nullptr;
    const Instance* targetInstanceValue = target
        ? target->audioNodeInstance() : nullptr;
    return parent && source && target && sourceInstanceValue &&
        targetInstanceValue && getRootAncestor() ==
            sourceInstanceValue->getRootAncestor() &&
        getRootAncestor() == targetInstanceValue->getRootAncestor() &&
        source->hasOutputPin(sourceName) && target->hasInputPin(targetName);
}

void Wire::renameToDefault()
{
    const AudioNode* source = asAudioNode(getSourceInstance());
    const AudioNode* target = asAudioNode(getTargetInstance());
    const std::vector<std::string> outputs = source
        ? source->outputPins() : std::vector<std::string>();
    const std::vector<std::string> inputs = target
        ? target->inputPins() : std::vector<std::string>();
    if (!outputs.empty())
        setSourceName(outputs.front());
    if (!inputs.empty())
        setTargetName(inputs.front());
}

void Wire::notifyEndpoint(AudioNode* endpoint, bool connected,
    const std::string& pin, Instance* other)
{
    if (!endpoint)
        return;
    boost::shared_ptr<Instance> wire;
    boost::shared_ptr<Instance> peer;
    try
    {
        wire = shared_from(static_cast<Instance*>(this));
        peer = shared_from(other);
    }
    catch (const boost::bad_weak_ptr&)
    {
        // Instance teardown can propagate an ancestry change after the last
        // owning shared_ptr has begun disposal. The disconnect event remains
        // meaningful even though the expiring Wire can no longer be retained.
    }
    endpoint->fireWiringChanged(connected, pin, wire, peer);
}

void Wire::notifyEndpoints(bool connected)
{
    AudioNode* source = asAudioNode(getSourceInstance());
    AudioNode* target = asAudioNode(getTargetInstance());
    notifyEndpoint(source, connected, sourceName, getTargetInstance());
    if (target != source || targetName != sourceName)
        notifyEndpoint(target, connected, targetName, getSourceInstance());
}

void Wire::onAncestorChanged(const AncestorChanged& event)
{
    Super::onAncestorChanged(event);
    raisePropertyChanged(propWireConnected);
    notifyEndpoints(getConnected());
}

AudioDeviceOutput::AudioDeviceOutput()
    : DescribedCreatable<AudioDeviceOutput, Instance, sAudioDeviceOutput>(
        sAudioDeviceOutput)
{
}

Network::Player* AudioDeviceOutput::getPlayer() const { return player.lock().get(); }

void AudioDeviceOutput::setPlayer(Network::Player* value)
{
    if (getPlayer() == value)
        return;
    player = value ? weak_from(value) : boost::weak_ptr<Network::Player>();
    raisePropertyChanged(propAudioDeviceOutputPlayer);
}

boost::shared_ptr<const Instances>
AudioDeviceOutput::getConnectedWiresReflection(std::string pin)
{
    return getConnectedWires(pin);
}

boost::shared_ptr<const Reflection::ValueArray>
AudioDeviceOutput::getInputPinsReflection()
{
    return getInputPins();
}

boost::shared_ptr<const Reflection::ValueArray>
AudioDeviceOutput::getOutputPinsReflection()
{
    return getOutputPins();
}

std::vector<std::string> AudioDeviceOutput::inputPins() const { return {"Input"}; }
std::vector<std::string> AudioDeviceOutput::outputPins() const { return {}; }

AudioFader::AudioFader()
    : DescribedCreatable<AudioFader, Instance, sAudioFader>(sAudioFader)
    , bypass(false)
    , volume(1.0f)
{
}

bool AudioFader::getBypass() const { return bypass; }
float AudioFader::getVolume() const { return volume; }

void AudioFader::setBypass(bool value)
{
    if (bypass == value)
        return;
    bypass = value;
    raisePropertyChanged(propAudioFaderBypass);
}

void AudioFader::setVolume(float value)
{
    if (!std::isfinite(value))
        throw std::runtime_error("AudioFader.Volume must be finite");
    value = std::clamp(value, 0.0f, 3.0f);
    if (volume == value)
        return;
    volume = value;
    raisePropertyChanged(propAudioFaderVolume);
}

boost::shared_ptr<const Instances>
AudioFader::getConnectedWiresReflection(std::string pin)
{
    return getConnectedWires(pin);
}

boost::shared_ptr<const Reflection::ValueArray>
AudioFader::getInputPinsReflection()
{
    return getInputPins();
}

boost::shared_ptr<const Reflection::ValueArray>
AudioFader::getOutputPinsReflection()
{
    return getOutputPins();
}

std::vector<std::string> AudioFader::inputPins() const { return {"Input"}; }
std::vector<std::string> AudioFader::outputPins() const { return {"Output"}; }

AudioDistortion::AudioDistortion()
    : DescribedCreatable<AudioDistortion, Instance, sAudioDistortion>(
        sAudioDistortion)
    , bypass(false)
    , level(0.5f)
{
}

bool AudioDistortion::getBypass() const { return bypass; }
float AudioDistortion::getLevel() const { return level; }

void AudioDistortion::setBypass(bool value)
{
    if (bypass == value)
        return;
    bypass = value;
    raisePropertyChanged(propAudioDistortionBypass);
}

void AudioDistortion::setLevel(float value)
{
    if (!std::isfinite(value))
        throw std::runtime_error("AudioDistortion.Level must be finite");
    value = std::clamp(value, 0.0f, 1.0f);
    if (level == value)
        return;
    level = value;
    raisePropertyChanged(propAudioDistortionLevel);
}

boost::shared_ptr<const Instances>
AudioDistortion::getConnectedWiresReflection(std::string pin)
{
    return getConnectedWires(pin);
}

boost::shared_ptr<const Reflection::ValueArray>
AudioDistortion::getInputPinsReflection()
{
    return getInputPins();
}

boost::shared_ptr<const Reflection::ValueArray>
AudioDistortion::getOutputPinsReflection()
{
    return getOutputPins();
}

std::vector<std::string> AudioDistortion::inputPins() const { return {"Input"}; }
std::vector<std::string> AudioDistortion::outputPins() const { return {"Output"}; }

AudioTremolo::AudioTremolo()
    : DescribedCreatable<AudioTremolo, Instance, sAudioTremolo>(sAudioTremolo)
    , bypass(false)
    , depth(0.5f)
    , duty(0.5f)
    , frequency(5.0f)
    , shape(0.5f)
    , skew(0.0f)
    , square(0.0f)
{
}

bool AudioTremolo::getBypass() const { return bypass; }
float AudioTremolo::getDepth() const { return depth; }
float AudioTremolo::getDuty() const { return duty; }
float AudioTremolo::getFrequency() const { return frequency; }
float AudioTremolo::getShape() const { return shape; }
float AudioTremolo::getSkew() const { return skew; }
float AudioTremolo::getSquare() const { return square; }

void AudioTremolo::setBypass(bool value)
{
    if (bypass == value)
        return;
    bypass = value;
    raisePropertyChanged(propAudioTremoloBypass);
}

namespace {
void setTremoloValue(AudioTremolo* tremolo, float& field, float value,
    float minimum, float maximum,
    const Reflection::PropertyDescriptor& descriptor, const char* name)
{
    if (!std::isfinite(value))
        throw std::runtime_error(std::string("AudioTremolo.") + name +
            " must be finite");
    value = std::clamp(value, minimum, maximum);
    if (field == value)
        return;
    field = value;
    tremolo->raisePropertyChanged(descriptor);
}
}

void AudioTremolo::setDepth(float value)
{
    setTremoloValue(this, depth, value, 0.0f, 1.0f,
        propAudioTremoloDepth, "Depth");
}
void AudioTremolo::setDuty(float value)
{
    setTremoloValue(this, duty, value, 0.0f, 1.0f,
        propAudioTremoloDuty, "Duty");
}
void AudioTremolo::setFrequency(float value)
{
    setTremoloValue(this, frequency, value, 0.0f, 20.0f,
        propAudioTremoloFrequency, "Frequency");
}
void AudioTremolo::setShape(float value)
{
    setTremoloValue(this, shape, value, 0.0f, 1.0f,
        propAudioTremoloShape, "Shape");
}
void AudioTremolo::setSkew(float value)
{
    setTremoloValue(this, skew, value, -1.0f, 1.0f,
        propAudioTremoloSkew, "Skew");
}
void AudioTremolo::setSquare(float value)
{
    setTremoloValue(this, square, value, 0.0f, 1.0f,
        propAudioTremoloSquare, "Square");
}

boost::shared_ptr<const Instances>
AudioTremolo::getConnectedWiresReflection(std::string pin)
{
    return getConnectedWires(pin);
}
boost::shared_ptr<const Reflection::ValueArray>
AudioTremolo::getInputPinsReflection()
{
    return getInputPins();
}
boost::shared_ptr<const Reflection::ValueArray>
AudioTremolo::getOutputPinsReflection()
{
    return getOutputPins();
}
std::vector<std::string> AudioTremolo::inputPins() const { return {"Input"}; }
std::vector<std::string> AudioTremolo::outputPins() const { return {"Output"}; }

#define RBX_DEFINE_AUDIO_MODULATION_CLASS(ClassName, ClassString, Prefix) \
ClassName::ClassName() \
    : DescribedCreatable<ClassName, Instance, ClassString>(ClassString) \
    , bypass(false), depth(0.5f), mix(0.5f), rate(1.0f) {} \
bool ClassName::getBypass() const { return bypass; } \
float ClassName::getDepth() const { return depth; } \
float ClassName::getMix() const { return mix; } \
float ClassName::getRate() const { return rate; } \
void ClassName::setBypass(bool value) \
{ \
    if (bypass == value) return; \
    bypass = value; \
    raisePropertyChanged(Prefix##Bypass); \
} \
void ClassName::setDepth(float value) \
{ \
    setModulationValue(this, depth, value, 0.0f, 1.0f, \
        Prefix##Depth, #ClassName ".Depth"); \
} \
void ClassName::setMix(float value) \
{ \
    setModulationValue(this, mix, value, 0.0f, 1.0f, \
        Prefix##Mix, #ClassName ".Mix"); \
} \
void ClassName::setRate(float value) \
{ \
    setModulationValue(this, rate, value, 0.0f, 20.0f, \
        Prefix##Rate, #ClassName ".Rate"); \
} \
boost::shared_ptr<const Instances> \
ClassName::getConnectedWiresReflection(std::string pin) \
{ return getConnectedWires(pin); } \
boost::shared_ptr<const Reflection::ValueArray> \
ClassName::getInputPinsReflection() { return getInputPins(); } \
boost::shared_ptr<const Reflection::ValueArray> \
ClassName::getOutputPinsReflection() { return getOutputPins(); } \
std::vector<std::string> ClassName::inputPins() const { return {"Input"}; } \
std::vector<std::string> ClassName::outputPins() const { return {"Output"}; }

namespace {
template<class Effect>
void setModulationValue(Effect* effect, float& field, float value,
    float minimum, float maximum,
    const Reflection::PropertyDescriptor& descriptor, const char* name)
{
    if (!std::isfinite(value))
        throw std::runtime_error(std::string(name) + " must be finite");
    value = std::clamp(value, minimum, maximum);
    if (field == value)
        return;
    field = value;
    effect->raisePropertyChanged(descriptor);
}
}

RBX_DEFINE_AUDIO_MODULATION_CLASS(AudioChorus, sAudioChorus, propAudioChorus)
RBX_DEFINE_AUDIO_MODULATION_CLASS(AudioFlanger, sAudioFlanger, propAudioFlanger)
#undef RBX_DEFINE_AUDIO_MODULATION_CLASS

AudioCompressor::AudioCompressor()
    : DescribedCreatable<AudioCompressor, Instance, sAudioCompressor>(
        sAudioCompressor)
    , attack(0.01f)
    , bypass(false)
    , editor(false)
    , makeupGain(0.0f)
    , ratio(4.0f)
    , release(0.1f)
    , threshold(-12.0f)
{
}

float AudioCompressor::getAttack() const { return attack; }
bool AudioCompressor::getBypass() const { return bypass; }
bool AudioCompressor::getEditor() const { return editor; }
float AudioCompressor::getMakeupGain() const { return makeupGain; }
float AudioCompressor::getRatio() const { return ratio; }
float AudioCompressor::getRelease() const { return release; }
float AudioCompressor::getThreshold() const { return threshold; }

void AudioCompressor::setBypass(bool value)
{
    if (bypass == value)
        return;
    bypass = value;
    raisePropertyChanged(propAudioCompressorBypass);
}

void AudioCompressor::setEditor(bool value)
{
    if (editor == value)
        return;
    editor = value;
    raisePropertyChanged(propAudioCompressorEditor);
}

namespace {
void setCompressorValue(AudioCompressor* compressor, float& field, float value,
    float minimum, float maximum,
    const Reflection::PropertyDescriptor& descriptor, const char* name)
{
    if (!std::isfinite(value))
        throw std::runtime_error(std::string("AudioCompressor.") + name +
            " must be finite");
    value = std::clamp(value, minimum, maximum);
    if (field == value)
        return;
    field = value;
    compressor->raisePropertyChanged(descriptor);
}
}

void AudioCompressor::setAttack(float value)
{
    setCompressorValue(this, attack, value, 0.0001f, 0.5f,
        propAudioCompressorAttack, "Attack");
}
void AudioCompressor::setMakeupGain(float value)
{
    setCompressorValue(this, makeupGain, value, -30.0f, 30.0f,
        propAudioCompressorMakeupGain, "MakeupGain");
}
void AudioCompressor::setRatio(float value)
{
    setCompressorValue(this, ratio, value, 1.0f, 50.0f,
        propAudioCompressorRatio, "Ratio");
}
void AudioCompressor::setRelease(float value)
{
    setCompressorValue(this, release, value, 0.01f, 5.0f,
        propAudioCompressorRelease, "Release");
}
void AudioCompressor::setThreshold(float value)
{
    setCompressorValue(this, threshold, value, -60.0f, 0.0f,
        propAudioCompressorThreshold, "Threshold");
}

boost::shared_ptr<const Instances>
AudioCompressor::getConnectedWiresReflection(std::string pin)
{
    return getConnectedWires(pin);
}
boost::shared_ptr<const Reflection::ValueArray>
AudioCompressor::getInputPinsReflection() { return getInputPins(); }
boost::shared_ptr<const Reflection::ValueArray>
AudioCompressor::getOutputPinsReflection() { return getOutputPins(); }
std::vector<std::string> AudioCompressor::inputPins() const { return {"Input"}; }
std::vector<std::string> AudioCompressor::outputPins() const { return {"Output"}; }

namespace {
const std::vector<std::string>& channelPins()
{
    static const std::vector<std::string> pins = {
        "Left", "Right", "Center", "SurroundLeft", "SurroundRight",
        "BackLeft", "BackRight", "Sub", "TopLeft", "TopRight",
        "TopBackLeft", "TopBackRight"};
    return pins;
}
}

AudioChannelMixer::AudioChannelMixer()
    : DescribedCreatable<AudioChannelMixer, Instance, sAudioChannelMixer>(
        sAudioChannelMixer)
    , layout(AUDIO_CHANNEL_STEREO)
{
}
AudioChannelLayout AudioChannelMixer::getLayout() const { return layout; }
void AudioChannelMixer::setLayout(AudioChannelLayout value)
{
    if (layout == value) return;
    layout = value;
    raisePropertyChanged(propAudioChannelMixerLayout);
}
boost::shared_ptr<const Instances>
AudioChannelMixer::getConnectedWiresReflection(std::string pin)
{ return getConnectedWires(pin); }
boost::shared_ptr<const Reflection::ValueArray>
AudioChannelMixer::getInputPinsReflection() { return getInputPins(); }
boost::shared_ptr<const Reflection::ValueArray>
AudioChannelMixer::getOutputPinsReflection() { return getOutputPins(); }
std::vector<std::string> AudioChannelMixer::inputPins() const
{
    std::vector<std::string> pins{"Input"};
    pins.insert(pins.end(), channelPins().begin(), channelPins().end());
    return pins;
}
std::vector<std::string> AudioChannelMixer::outputPins() const { return {"Output"}; }

AudioChannelSplitter::AudioChannelSplitter()
    : DescribedCreatable<AudioChannelSplitter, Instance, sAudioChannelSplitter>(
        sAudioChannelSplitter)
    , layout(AUDIO_CHANNEL_STEREO)
{
}
AudioChannelLayout AudioChannelSplitter::getLayout() const { return layout; }
void AudioChannelSplitter::setLayout(AudioChannelLayout value)
{
    if (layout == value) return;
    layout = value;
    raisePropertyChanged(propAudioChannelSplitterLayout);
}
boost::shared_ptr<const Instances>
AudioChannelSplitter::getConnectedWiresReflection(std::string pin)
{ return getConnectedWires(pin); }
boost::shared_ptr<const Reflection::ValueArray>
AudioChannelSplitter::getInputPinsReflection() { return getInputPins(); }
boost::shared_ptr<const Reflection::ValueArray>
AudioChannelSplitter::getOutputPinsReflection() { return getOutputPins(); }
std::vector<std::string> AudioChannelSplitter::inputPins() const { return {"Input"}; }
std::vector<std::string> AudioChannelSplitter::outputPins() const
{
    std::vector<std::string> pins{"Output"};
    pins.insert(pins.end(), channelPins().begin(), channelPins().end());
    return pins;
}

AudioEmitter::AudioEmitter()
    : DescribedCreatable<AudioEmitter, Instance, sAudioEmitter>(sAudioEmitter)
    , acousticSimulationEnabled(true)
    , positionType(EMITTER_POSITION_PARENT)
    , diffractionEnabled(SIMULATION_DEFAULT)
    , occlusionEnabled(SIMULATION_DEFAULT)
    , reverbEnabled(SIMULATION_DEFAULT)
    , simulationFidelity(AUDIO_SIMULATION_AUTOMATIC)
    , attenuationVersion(0)
{
}

bool AudioEmitter::getAcousticSimulationEnabled() const
{
    return acousticSimulationEnabled;
}

void AudioEmitter::setAcousticSimulationEnabled(bool value)
{
    if (acousticSimulationEnabled == value)
        return;
    acousticSimulationEnabled = value;
    raisePropertyChanged(propAudioEmitterAcousticSimulationEnabled);
}

const std::string& AudioEmitter::getAudioInteractionGroup() const
{
    return audioInteractionGroup;
}

void AudioEmitter::setAudioInteractionGroup(const std::string& value)
{
    if (audioInteractionGroup == value)
        return;
    audioInteractionGroup = value;
    raisePropertyChanged(propAudioEmitterInteractionGroup);
}

Instance* AudioEmitter::getPositionInstance() const
{
    return positionInstance.lock().get();
}

void AudioEmitter::setPositionInstance(Instance* value)
{
    if (getPositionInstance() == value)
        return;
    positionInstance = value ? weak_from(value) : boost::weak_ptr<Instance>();
    raisePropertyChanged(propAudioEmitterPositionInstance);
}

EmitterPositionType AudioEmitter::getPositionType() const
{
    return positionType;
}

void AudioEmitter::setPositionType(EmitterPositionType value)
{
    if (positionType == value)
        return;
    positionType = value;
    raisePropertyChanged(propAudioEmitterPositionType);
}

SimulationMode AudioEmitter::getDiffractionEnabled() const { return diffractionEnabled; }
SimulationMode AudioEmitter::getOcclusionEnabled() const { return occlusionEnabled; }
SimulationMode AudioEmitter::getReverbEnabled() const { return reverbEnabled; }
AudioSimulationFidelity AudioEmitter::getSimulationFidelity() const { return simulationFidelity; }
void AudioEmitter::setDiffractionEnabled(SimulationMode value)
{ if (diffractionEnabled != value) { diffractionEnabled = value; raisePropertyChanged(propAudioEmitterDiffraction); } }
void AudioEmitter::setOcclusionEnabled(SimulationMode value)
{ if (occlusionEnabled != value) { occlusionEnabled = value; raisePropertyChanged(propAudioEmitterOcclusion); } }
void AudioEmitter::setReverbEnabled(SimulationMode value)
{ if (reverbEnabled != value) { reverbEnabled = value; raisePropertyChanged(propAudioEmitterReverb); } }
void AudioEmitter::setSimulationFidelity(AudioSimulationFidelity value)
{ if (simulationFidelity != value) { simulationFidelity = value; raisePropertyChanged(propAudioEmitterSimulationFidelity); } }

float AudioEmitter::getAudibilityFor(boost::shared_ptr<Instance> listenerValue)
{
    AudioListener* listener = Instance::fastDynamicCast<AudioListener>(listenerValue.get());
    return listener ? spatialAudibility(*this, *listener) : 0.0f;
}

boost::shared_ptr<const Instances> AudioEmitter::getInteractingListeners()
{
    boost::shared_ptr<Instances> result(new Instances());
    const Instance* root = getRootAncestor();
    if (!root) return result;
    root->visitDescendants([&](const boost::shared_ptr<Instance>& value) {
        AudioListener* listener = Instance::fastDynamicCast<AudioListener>(value.get());
        if (listener && listener->getAudioInteractionGroup() == audioInteractionGroup)
            result->push_back(value);
    });
    return result;
}

BinaryString AudioEmitter::getDistanceAttenuationData() const
{
    return distanceAttenuationData;
}

void AudioEmitter::setDistanceAttenuationData(const BinaryString& value)
{
    if (distanceAttenuationData == value)
        return;
    distanceAttenuationData = value;
    distanceAttenuationCurve.clear();
    decodeDistanceAttenuation();
    ++attenuationVersion;
    raisePropertyChanged(propAudioEmitterDistanceAttenuation);
}

const std::vector<Audio::AttenuationPoint>&
AudioEmitter::distanceAttenuation() const
{
    return distanceAttenuationCurve;
}

BinaryString AudioEmitter::getAngleAttenuationData() const
{
    return angleAttenuationData;
}

void AudioEmitter::setAngleAttenuationData(const BinaryString& value)
{
    if (angleAttenuationData == value)
        return;
    angleAttenuationData = value;
    angleAttenuationCurve.clear();
    decodeAngleAttenuation();
    ++attenuationVersion;
    raisePropertyChanged(propAudioEmitterAngleAttenuation);
}

const std::vector<Audio::AttenuationPoint>& AudioEmitter::angleAttenuation() const
{
    return angleAttenuationCurve;
}

int AudioEmitter::getDistanceAttenuationLua(lua_State* state)
{
    lua_createtable(state, 0,
        static_cast<int>(distanceAttenuationCurve.size()));
    for (const Audio::AttenuationPoint& point : distanceAttenuationCurve)
    {
        lua_pushnumber(state, point.distance);
        lua_pushnumber(state, point.gain);
        lua_settable(state, -3);
    }
    return 1;
}

int AudioEmitter::setDistanceAttenuationLua(lua_State* state)
{
    distanceAttenuationCurve = readCurve(state, false);
    encodeDistanceAttenuation();
    ++attenuationVersion;
    raisePropertyChanged(propAudioEmitterDistanceAttenuation);
    return 0;
}

int AudioEmitter::getAngleAttenuationLua(lua_State* state)
{
    lua_createtable(state, 0, static_cast<int>(angleAttenuationCurve.size()));
    for (const Audio::AttenuationPoint& point : angleAttenuationCurve)
    {
        lua_pushnumber(state, point.distance);
        lua_pushnumber(state, point.gain);
        lua_settable(state, -3);
    }
    return 1;
}

int AudioEmitter::setAngleAttenuationLua(lua_State* state)
{
    angleAttenuationCurve = readCurve(state, true);
    encodeAngleAttenuation();
    ++attenuationVersion;
    raisePropertyChanged(propAudioEmitterAngleAttenuation);
    return 0;
}

boost::shared_ptr<const Instances>
AudioEmitter::getConnectedWiresReflection(std::string pin)
{
    return getConnectedWires(pin);
}

boost::shared_ptr<const Reflection::ValueArray>
AudioEmitter::getInputPinsReflection()
{
    return getInputPins();
}

boost::shared_ptr<const Reflection::ValueArray>
AudioEmitter::getOutputPinsReflection()
{
    return getOutputPins();
}

std::vector<std::string> AudioEmitter::inputPins() const { return {"Input"}; }
std::vector<std::string> AudioEmitter::outputPins() const { return {}; }

void AudioEmitter::encodeDistanceAttenuation()
{
    std::string bytes;
    bytes.reserve(8 + distanceAttenuationCurve.size() * 8);
    const auto append32 = [&bytes](std::uint32_t value) {
        for (unsigned shift = 0; shift != 32; shift += 8)
            bytes.push_back(static_cast<char>((value >> shift) & 0xff));
    };
    append32(0x31434152); // "RAC1" in little-endian storage.
    append32(static_cast<std::uint32_t>(distanceAttenuationCurve.size()));
    for (const Audio::AttenuationPoint& point : distanceAttenuationCurve)
    {
        append32(std::bit_cast<std::uint32_t>(point.distance));
        append32(std::bit_cast<std::uint32_t>(point.gain));
    }
    distanceAttenuationData = BinaryString(bytes);
}

bool AudioEmitter::decodeDistanceAttenuation()
{
    const std::string& bytes = distanceAttenuationData.value();
    const auto read32 = [&bytes](std::size_t offset, std::uint32_t& value) {
        if (offset + 4 > bytes.size())
            return false;
        value = static_cast<unsigned char>(bytes[offset]) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 8 |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 16 |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3])) << 24;
        return true;
    };
    std::uint32_t magic = 0;
    std::uint32_t count = 0;
    if (!read32(0, magic) || !read32(4, count) || magic != 0x31434152 ||
        count > 400 || bytes.size() != 8 + count * 8)
        return false;
    std::vector<Audio::AttenuationPoint> curve;
    curve.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        std::uint32_t distanceBits = 0;
        std::uint32_t gainBits = 0;
        if (!read32(8 + index * 8, distanceBits) ||
            !read32(12 + index * 8, gainBits))
            return false;
        const float distance = std::bit_cast<float>(distanceBits);
        const float gain = std::bit_cast<float>(gainBits);
        if (!std::isfinite(distance) || !std::isfinite(gain) ||
            distance < 0.0f || gain < 0.0f || gain > 1.0f ||
            (!curve.empty() && curve.back().distance >= distance))
            return false;
        curve.push_back({distance, gain});
    }
    distanceAttenuationCurve = std::move(curve);
    return true;
}

void AudioEmitter::encodeAngleAttenuation()
{
    std::string bytes;
    bytes.reserve(8 + angleAttenuationCurve.size() * 8);
    const auto append32 = [&bytes](std::uint32_t value) {
        for (unsigned shift = 0; shift != 32; shift += 8)
            bytes.push_back(static_cast<char>((value >> shift) & 0xff));
    };
    append32(0x31414152); // "RAA1" in little-endian storage.
    append32(static_cast<std::uint32_t>(angleAttenuationCurve.size()));
    for (const Audio::AttenuationPoint& point : angleAttenuationCurve)
    {
        append32(std::bit_cast<std::uint32_t>(point.distance));
        append32(std::bit_cast<std::uint32_t>(point.gain));
    }
    angleAttenuationData = BinaryString(bytes);
}

bool AudioEmitter::decodeAngleAttenuation()
{
    const std::string& bytes = angleAttenuationData.value();
    const auto read32 = [&bytes](std::size_t offset, std::uint32_t& value) {
        if (offset + 4 > bytes.size())
            return false;
        value = static_cast<unsigned char>(bytes[offset]) |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 1])) << 8 |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 2])) << 16 |
            static_cast<std::uint32_t>(static_cast<unsigned char>(bytes[offset + 3])) << 24;
        return true;
    };
    std::uint32_t magic = 0;
    std::uint32_t count = 0;
    if (!read32(0, magic) || !read32(4, count) || magic != 0x31414152 ||
        count > 400 || bytes.size() != 8 + count * 8)
        return false;
    std::vector<Audio::AttenuationPoint> curve;
    curve.reserve(count);
    for (std::uint32_t index = 0; index < count; ++index)
    {
        std::uint32_t angleBits = 0;
        std::uint32_t gainBits = 0;
        if (!read32(8 + index * 8, angleBits) ||
            !read32(12 + index * 8, gainBits))
            return false;
        const float angle = std::bit_cast<float>(angleBits);
        const float gain = std::bit_cast<float>(gainBits);
        if (!std::isfinite(angle) || !std::isfinite(gain) || angle < 0.0f ||
            angle > 180.0f || gain < 0.0f || gain > 1.0f ||
            (!curve.empty() && curve.back().distance >= angle))
            return false;
        curve.push_back({angle, gain});
    }
    angleAttenuationCurve = std::move(curve);
    return true;
}

AudioListener::AudioListener()
    : Super(sAudioListener)
    , acousticSimulationEnabled(true)
    , positionType(LISTENER_POSITION_PARENT)
    , diffractionEnabled(SIMULATION_DEFAULT)
    , occlusionEnabled(SIMULATION_DEFAULT)
    , reverbEnabled(SIMULATION_DEFAULT)
    , simulationFidelity(AUDIO_SIMULATION_AUTOMATIC)
{
}

bool AudioListener::getAcousticSimulationEnabled() const { return acousticSimulationEnabled; }
const std::string& AudioListener::getAudioInteractionGroup() const { return audioInteractionGroup; }
Instance* AudioListener::getPositionInstance() const { return positionInstance.lock().get(); }
ListenerPositionType AudioListener::getPositionType() const { return positionType; }
SimulationMode AudioListener::getDiffractionEnabled() const { return diffractionEnabled; }
SimulationMode AudioListener::getOcclusionEnabled() const { return occlusionEnabled; }
SimulationMode AudioListener::getReverbEnabled() const { return reverbEnabled; }
AudioSimulationFidelity AudioListener::getSimulationFidelity() const { return simulationFidelity; }

void AudioListener::setAcousticSimulationEnabled(bool value)
{ if (acousticSimulationEnabled != value) { acousticSimulationEnabled = value; raisePropertyChanged(propAudioListenerAcoustic); } }
void AudioListener::setAudioInteractionGroup(const std::string& value)
{ if (audioInteractionGroup != value) { audioInteractionGroup = value; raisePropertyChanged(propAudioListenerInteractionGroup); } }
void AudioListener::setPositionInstance(Instance* value)
{ if (getPositionInstance() != value) { positionInstance = value ? weak_from(value) : boost::weak_ptr<Instance>(); raisePropertyChanged(propAudioListenerPositionInstance); } }
void AudioListener::setPositionType(ListenerPositionType value)
{ if (positionType != value) { positionType = value; raisePropertyChanged(propAudioListenerPositionType); } }
void AudioListener::setDiffractionEnabled(SimulationMode value)
{ if (diffractionEnabled != value) { diffractionEnabled = value; raisePropertyChanged(propAudioListenerDiffraction); } }
void AudioListener::setOcclusionEnabled(SimulationMode value)
{ if (occlusionEnabled != value) { occlusionEnabled = value; raisePropertyChanged(propAudioListenerOcclusion); } }
void AudioListener::setReverbEnabled(SimulationMode value)
{ if (reverbEnabled != value) { reverbEnabled = value; raisePropertyChanged(propAudioListenerReverb); } }
void AudioListener::setSimulationFidelity(AudioSimulationFidelity value)
{ if (simulationFidelity != value) { simulationFidelity = value; raisePropertyChanged(propAudioListenerSimulationFidelity); } }

BinaryString AudioListener::getDistanceAttenuationData() const { return distanceAttenuationData; }
BinaryString AudioListener::getAngleAttenuationData() const { return angleAttenuationData; }
void AudioListener::setDistanceAttenuationData(const BinaryString& value)
{
    if (distanceAttenuationData == value) return;
    std::vector<Audio::AttenuationPoint> decoded;
    if (!value.value().empty() && !decodeCurve(value, decoded, false)) decoded.clear();
    distanceAttenuationData = value;
    distanceAttenuationCurve = std::move(decoded);
    raisePropertyChanged(propAudioListenerDistance);
}
void AudioListener::setAngleAttenuationData(const BinaryString& value)
{
    if (angleAttenuationData == value) return;
    std::vector<Audio::AttenuationPoint> decoded;
    if (!value.value().empty() && !decodeCurve(value, decoded, true)) decoded.clear();
    angleAttenuationData = value;
    angleAttenuationCurve = std::move(decoded);
    raisePropertyChanged(propAudioListenerAngle);
}
int AudioListener::getDistanceAttenuationLua(lua_State* state)
{ return pushCurve(state, distanceAttenuationCurve); }
int AudioListener::getAngleAttenuationLua(lua_State* state)
{ return pushCurve(state, angleAttenuationCurve); }
int AudioListener::setDistanceAttenuationLua(lua_State* state)
{
    std::vector<Audio::AttenuationPoint> curve = readCurve(state, false);
    distanceAttenuationCurve = std::move(curve);
    distanceAttenuationData = encodeCurve(distanceAttenuationCurve);
    raisePropertyChanged(propAudioListenerDistance);
    return 0;
}
int AudioListener::setAngleAttenuationLua(lua_State* state)
{
    std::vector<Audio::AttenuationPoint> curve = readCurve(state, true);
    angleAttenuationCurve = std::move(curve);
    angleAttenuationData = encodeCurve(angleAttenuationCurve);
    raisePropertyChanged(propAudioListenerAngle);
    return 0;
}
const std::vector<Audio::AttenuationPoint>& AudioListener::distanceAttenuation() const
{ return distanceAttenuationCurve; }
const std::vector<Audio::AttenuationPoint>& AudioListener::angleAttenuation() const
{ return angleAttenuationCurve; }
float AudioListener::getAudibilityFor(boost::shared_ptr<Instance> emitterValue)
{
    AudioEmitter* emitter = Instance::fastDynamicCast<AudioEmitter>(emitterValue.get());
    return emitter ? spatialAudibility(*emitter, *this) : 0.0f;
}
boost::shared_ptr<const Instances> AudioListener::getInteractingEmitters()
{
    boost::shared_ptr<Instances> result(new Instances());
    const Instance* root = getRootAncestor();
    if (!root) return result;
    root->visitDescendants([&](const boost::shared_ptr<Instance>& value) {
        AudioEmitter* emitter = Instance::fastDynamicCast<AudioEmitter>(value.get());
        if (emitter && emitter->getAudioInteractionGroup() == audioInteractionGroup)
            result->push_back(value);
    });
    return result;
}
void AudioListener::reset()
{
    setAcousticSimulationEnabled(true);
    setAudioInteractionGroup(std::string());
    setPositionInstance(nullptr);
    setPositionType(LISTENER_POSITION_PARENT);
    setDiffractionEnabled(SIMULATION_DEFAULT);
    setOcclusionEnabled(SIMULATION_DEFAULT);
    setReverbEnabled(SIMULATION_DEFAULT);
    setSimulationFidelity(AUDIO_SIMULATION_AUTOMATIC);
    setDistanceAttenuationData(BinaryString());
    setAngleAttenuationData(BinaryString());
}
Audio::ListenerState AudioListener::listenerState() const
{
    Audio::ListenerState result;
    Instance* source = const_cast<Instance*>(positionType == LISTENER_POSITION_INSTANCE
        ? getPositionInstance() : getParent());
    CoordinateFrame frame;
    if (worldFrame(source, frame))
    {
        result.position = {frame.translation.x, frame.translation.y, frame.translation.z};
        const Vector3 direction = -frame.lookVector();
        const Vector3 up = frame.upVector();
        result.direction = {direction.x, direction.y, direction.z};
        result.up = {up.x, up.y, up.z};
        if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(source))
        {
            const Vector3 velocity = part->getVelocity().linear;
            result.velocity = {velocity.x, velocity.y, velocity.z};
        }
    }
    result.distanceAttenuationCurve = distanceAttenuationCurve;
    result.angleAttenuationCurve = angleAttenuationCurve;
    return result;
}
boost::shared_ptr<const Instances> AudioListener::getConnectedWiresReflection(std::string pin)
{ return getConnectedWires(pin); }
boost::shared_ptr<const Reflection::ValueArray> AudioListener::getInputPinsReflection()
{ return getInputPins(); }
boost::shared_ptr<const Reflection::ValueArray> AudioListener::getOutputPinsReflection()
{ return getOutputPins(); }
std::vector<std::string> AudioListener::inputPins() const { return {}; }
std::vector<std::string> AudioListener::outputPins() const { return {"Output"}; }
void AudioListener::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{ Super::onServiceProvider(oldProvider, newProvider); }

} // namespace RBX::Soundscape
