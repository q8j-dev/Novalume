#include "V8DataModel/FactoryRegistration.h"
#include "V8DataModel/Folder.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/ProximityPrompt.h"
#include "V8DataModel/TextBox.h"
#include "V8DataModel/TextButton.h"
#include "V8DataModel/TextLabel.h"
#include "V8Xml/SerializerBinary.h"
#include "V8Xml/Serializer.h"
#include "V8Xml/SerializerV2.h"
#include "Script/Script.h"
#include "V8Tree/Verb.h"
#include "Security/SecurityContext.h"
#include "audio/AudioGraph.h"
#include "audio/SoundService.h"
#include "lua/lua.hpp"

#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>

namespace {

struct Inventory
{
    std::size_t instances = 0;
    std::size_t parts = 0;
    std::size_t scripts = 0;
    std::size_t prompts = 0;
    std::size_t fontFaces = 0;
    float promptHoldDuration = -1;
    float promptActivationDistance = -1;
    int promptKeyboardKey = -1;
    int promptStyle = -1;
};

Inventory inventory(RBX::DataModel& dataModel)
{
    Inventory result;
    const auto inspect = [&result](const boost::shared_ptr<RBX::Instance>& instance) {
        ++result.instances;
        if (instance->isA<RBX::PartInstance>()) ++result.parts;
        if (instance->isA<RBX::BaseScript>()) ++result.scripts;
        if (RBX::ProximityPrompt* prompt =
                RBX::Instance::fastDynamicCast<RBX::ProximityPrompt>(instance.get())) {
            ++result.prompts;
            result.promptHoldDuration = prompt->getHoldDuration();
            result.promptActivationDistance = prompt->getMaxActivationDistance();
            result.promptKeyboardKey = static_cast<int>(prompt->getKeyboardKeyCode());
            result.promptStyle = static_cast<int>(prompt->getStyle());
        }
        const RBX::Font* font = nullptr;
        if (RBX::TextLabel* text = RBX::Instance::fastDynamicCast<RBX::TextLabel>(instance.get()))
            font = &text->getFontFace();
        else if (RBX::GuiTextButton* text = RBX::Instance::fastDynamicCast<RBX::GuiTextButton>(instance.get()))
            font = &text->getFontFace();
        else if (RBX::TextBox* text = RBX::Instance::fastDynamicCast<RBX::TextBox>(instance.get()))
            font = &text->getFontFace();
        if (font && !font->getFamily().empty()) ++result.fontFaces;
    };
    dataModel.visitDescendants(inspect);
    return result;
}

void loadPlace(const char* path, RBX::DataModel& dataModel)
{
    std::ifstream stream(path, std::ios::binary);
    if (!stream)
        throw std::runtime_error(std::string("could not open selected place: ") + path);
    RBX::Security::Impersonator permission(RBX::Security::COM);
    RBX::DataModel::LegacyLock lock(&dataModel, RBX::DataModelJob::Write);
    Serializer serializer;
    serializer.load(stream, &dataModel);
}

Inventory addedInventory(const Inventory& after, const Inventory& before)
{
    Inventory result = after;
    result.instances -= before.instances;
    result.parts -= before.parts;
    result.scripts -= before.scripts;
    result.prompts -= before.prompts;
    result.fontFaces -= before.fontFaces;
    return result;
}

} // namespace

int main(int argc, char** argv)
{
    if (argc != 2)
        throw std::runtime_error("expected one selected modern place");
    static std::once_flag registration;
    std::call_once(registration, [] { static RBX::FactoryRegistrator registrator; });

    boost::shared_ptr<RBX::DataModel> dataModel = RBX::DataModel::createDataModel(
        true, new RBX::NullVerb(nullptr, ""), false);
    RBX::ServiceProvider::create<RBX::ProximityPromptService>(dataModel.get());
    RBX::Soundscape::SoundService* soundService =
        RBX::ServiceProvider::create<RBX::Soundscape::SoundService>(
            dataModel.get());
    const RBX::CoordinateFrame authoredListener(
        RBX::Vector3(4.0f, 5.0f, 6.0f));
    soundService->setListenerCFrame(authoredListener);
    soundService->setListenerType(RBX::Soundscape::CFrame);
    if (soundService->getDistanceFactor() != 3.33f ||
        soundService->getListenerType() != RBX::Soundscape::CFrame ||
        soundService->getListenerCFrame() != authoredListener)
        throw std::runtime_error("current SoundService listener properties failed");
    const Inventory baseline = inventory(*dataModel);
    loadPlace(argv[1], *dataModel);
    const Inventory afterBinary = inventory(*dataModel);
    const Inventory binary = addedInventory(afterBinary, baseline);
    if (binary.instances < 4000 || binary.parts < 700 || binary.scripts < 10 ||
        binary.prompts != 1 || binary.fontFaces < 10 ||
        binary.promptHoldDuration != 1.0f ||
        binary.promptActivationDistance != 5.0f ||
        binary.promptKeyboardKey != RBX::SDLK_e ||
        binary.promptStyle != RBX::ProximityPrompt::STYLE_CUSTOM)
        throw std::runtime_error("selected place inventory is unexpectedly incomplete");

    boost::shared_ptr<RBX::Folder> sourceRoot =
        RBX::Creatable<RBX::Instance>::create<RBX::Folder>();
    boost::shared_ptr<RBX::TextLabel> source =
        RBX::Creatable<RBX::Instance>::create<RBX::TextLabel>();
    source->setFontFace(RBX::Font(
        "rbxasset://fonts/families/BuilderSans.json",
        RBX::FONT_WEIGHT_SEMI_BOLD, RBX::FONT_STYLE_ITALIC));
    source->setParent(sourceRoot.get());
    std::stringstream encoded;
    RBX::SerializerBinary::serialize(encoded, sourceRoot.get());
    boost::shared_ptr<RBX::Folder> decodedRoot =
        RBX::Creatable<RBX::Instance>::create<RBX::Folder>();
    RBX::SerializerBinary::deserialize(encoded, decodedRoot.get());
    RBX::TextLabel* decoded = decodedRoot->findFirstChildOfType<RBX::TextLabel>();
    if (!decoded || decoded->getFontFace() != source->getFontFace())
        throw std::runtime_error("binary FontFace round trip changed semantic fields");

    boost::shared_ptr<RBX::Folder> audioRoot =
        RBX::Creatable<RBX::Instance>::create<RBX::Folder>();
    boost::shared_ptr<RBX::Soundscape::AudioDeviceOutput> audioOutput =
        RBX::Creatable<RBX::Instance>::create<
            RBX::Soundscape::AudioDeviceOutput>();
    audioOutput->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioEmitter> audioEmitter =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::AudioEmitter>();
    audioEmitter->setPositionType(
        RBX::Soundscape::EMITTER_POSITION_INSTANCE);
    audioEmitter->setParent(audioRoot.get());
    boost::shared_ptr<RBX::PartInstance> emitterPart =
        RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
    emitterPart->setCoordinateFrameRoot(RBX::CoordinateFrame(
        RBX::Vector3(0.0f, 0.0f, 0.0f)));
    emitterPart->setParent(audioRoot.get());
    audioEmitter->setPositionInstance(emitterPart.get());
    boost::shared_ptr<RBX::PartInstance> listenerPart =
        RBX::Creatable<RBX::Instance>::create<RBX::BasicPartInstance>();
    listenerPart->setCoordinateFrameRoot(RBX::CoordinateFrame(
        RBX::Vector3(10.0f, 0.0f, 0.0f)));
    listenerPart->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioListener> audioListener =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::AudioListener>();
    audioListener->setPositionType(RBX::Soundscape::LISTENER_POSITION_INSTANCE);
    audioListener->setPositionInstance(listenerPart.get());
    audioListener->setAudioInteractionGroup("World");
    audioEmitter->setAudioInteractionGroup("World");
    audioListener->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioPlayer> audioPlayer =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::AudioPlayer>();
    audioPlayer->setAssetId("rbxasset://sounds/uuhhh.mp3");
    audioPlayer->setAudioContent(
        RBX::Content::fromUri("rbxassetid://1843529274"));
    audioPlayer->setLoopRegion(RBX::NumberRange(0.1f, 0.3f));
    audioPlayer->setPlaybackRegion(RBX::NumberRange(0.05f, 0.4f));
    audioPlayer->setPlaybackSpeed(25.0);
    audioPlayer->setVolume(12.0f);
    if (audioPlayer->getPlaybackSpeed() != 20.0 ||
        audioPlayer->getVolume() != 10.0f)
        throw std::runtime_error(
            "AudioPlayer did not enforce current playback/volume bounds");
    audioPlayer->setPlaybackSpeed(1.0);
    audioPlayer->setVolume(1.0f);
    audioPlayer->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioFader> audioFader =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::AudioFader>();
    audioFader->setVolume(4.0f);
    if (audioFader->getVolume() != 3.0f)
        throw std::runtime_error("AudioFader did not enforce its current volume bound");
    audioFader->setVolume(0.25f);
    audioFader->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioDistortion> audioDistortion =
        RBX::Creatable<RBX::Instance>::create<
            RBX::Soundscape::AudioDistortion>();
    audioDistortion->setLevel(2.0f);
    if (audioDistortion->getLevel() != 1.0f)
        throw std::runtime_error(
            "AudioDistortion did not enforce its current level bound");
    audioDistortion->setLevel(0.35f);
    audioDistortion->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioTremolo> audioTremolo =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::AudioTremolo>();
    audioTremolo->setDepth(2.0f);
    audioTremolo->setDuty(-1.0f);
    audioTremolo->setFrequency(25.0f);
    audioTremolo->setSkew(-2.0f);
    if (audioTremolo->getDepth() != 1.0f ||
        audioTremolo->getDuty() != 0.0f ||
        audioTremolo->getFrequency() != 20.0f ||
        audioTremolo->getSkew() != -1.0f)
        throw std::runtime_error(
            "AudioTremolo did not enforce current metadata bounds");
    audioTremolo->setDepth(0.7f);
    audioTremolo->setDuty(0.8f);
    audioTremolo->setFrequency(6.0f);
    audioTremolo->setShape(0.25f);
    audioTremolo->setSkew(-0.2f);
    audioTremolo->setSquare(0.4f);
    audioTremolo->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioChorus> audioChorus =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::AudioChorus>();
    audioChorus->setDepth(0.6f);
    audioChorus->setMix(0.3f);
    audioChorus->setRate(4.0f);
    audioChorus->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioFlanger> audioFlanger =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::AudioFlanger>();
    audioFlanger->setDepth(0.4f);
    audioFlanger->setMix(0.2f);
    audioFlanger->setRate(3.0f);
    audioFlanger->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioChannelMixer> audioMixer =
        RBX::Creatable<RBX::Instance>::create<
            RBX::Soundscape::AudioChannelMixer>();
    audioMixer->setLayout(RBX::Soundscape::AUDIO_CHANNEL_QUAD);
    audioMixer->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::AudioChannelSplitter> audioSplitter =
        RBX::Creatable<RBX::Instance>::create<
            RBX::Soundscape::AudioChannelSplitter>();
    audioSplitter->setLayout(RBX::Soundscape::AUDIO_CHANNEL_QUAD);
    audioSplitter->setParent(audioRoot.get());
    boost::shared_ptr<RBX::Soundscape::Wire> emitterWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    emitterWire->setName("EmitterWire");
    emitterWire->setSourceInstance(audioPlayer.get());
    emitterWire->setTargetInstance(audioFader.get());
    emitterWire->setParent(audioPlayer.get());
    boost::shared_ptr<RBX::Soundscape::Wire> faderWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    faderWire->setName("FaderWire");
    faderWire->setSourceInstance(audioFader.get());
    faderWire->setTargetInstance(audioDistortion.get());
    faderWire->setParent(audioFader.get());
    boost::shared_ptr<RBX::Soundscape::Wire> distortionWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    distortionWire->setName("DistortionWire");
    distortionWire->setSourceInstance(audioDistortion.get());
    distortionWire->setTargetInstance(audioTremolo.get());
    distortionWire->setParent(audioDistortion.get());
    boost::shared_ptr<RBX::Soundscape::Wire> tremoloWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    tremoloWire->setName("TremoloWire");
    tremoloWire->setSourceInstance(audioTremolo.get());
    tremoloWire->setTargetInstance(audioChorus.get());
    tremoloWire->setParent(audioTremolo.get());
    boost::shared_ptr<RBX::Soundscape::Wire> chorusWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    chorusWire->setName("ChorusWire");
    chorusWire->setSourceInstance(audioChorus.get());
    chorusWire->setTargetInstance(audioFlanger.get());
    chorusWire->setParent(audioChorus.get());
    boost::shared_ptr<RBX::Soundscape::Wire> flangerWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    flangerWire->setName("FlangerWire");
    flangerWire->setSourceInstance(audioFlanger.get());
    flangerWire->setTargetInstance(audioMixer.get());
    flangerWire->setParent(audioFlanger.get());
    boost::shared_ptr<RBX::Soundscape::Wire> mixerWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    mixerWire->setName("MixerWire");
    mixerWire->setSourceInstance(audioMixer.get());
    mixerWire->setTargetInstance(audioSplitter.get());
    mixerWire->setParent(audioMixer.get());
    boost::shared_ptr<RBX::Soundscape::Wire> splitterWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    splitterWire->setName("SplitterWire");
    splitterWire->setSourceInstance(audioSplitter.get());
    splitterWire->setSourceName("Output");
    splitterWire->setTargetInstance(audioEmitter.get());
    splitterWire->setParent(audioSplitter.get());
    if (!emitterWire->getConnected() || emitterWire->getSourceName() != "Output" ||
        emitterWire->getTargetName() != "Input" ||
        audioPlayer->getConnectedWiresReflection("Output")->size() != 1 ||
        audioFader->getConnectedWiresReflection("Input")->size() != 1 ||
        audioFader->getConnectedWiresReflection("Output")->size() != 1 ||
        audioMixer->getInputPinsReflection()->size() != 13 ||
        audioMixer->getOutputPinsReflection()->size() != 1 ||
        audioSplitter->getInputPinsReflection()->size() != 1 ||
        audioSplitter->getOutputPinsReflection()->size() != 13 ||
        audioMixer->getConnectedWiresReflection("Input")->size() != 1 ||
        audioMixer->getConnectedWiresReflection("Output")->size() != 1 ||
        audioSplitter->getConnectedWiresReflection("Input")->size() != 1 ||
        audioSplitter->getConnectedWiresReflection("Output")->size() != 1 ||
        audioEmitter->getConnectedWiresReflection("Input")->size() != 1 ||
        !faderWire->getConnected() || !mixerWire->getConnected() ||
        !splitterWire->getConnected())
        throw std::runtime_error(
            "combined AudioPlayer channel graph did not connect");
    lua_State* attenuationState = luaL_newstate();
    if (!attenuationState)
        throw std::runtime_error("could not create attenuation contract state");
    lua_pushnil(attenuationState);
    lua_newtable(attenuationState);
    lua_pushnumber(attenuationState, 5.0);
    lua_pushnumber(attenuationState, 1.0);
    lua_settable(attenuationState, 2);
    lua_pushnumber(attenuationState, 150.0);
    lua_pushnumber(attenuationState, 0.0);
    lua_settable(attenuationState, 2);
    audioEmitter->setDistanceAttenuationLua(attenuationState);
    lua_settop(attenuationState, 0);
    lua_pushnil(attenuationState);
    if (audioEmitter->getDistanceAttenuationLua(attenuationState) != 1 ||
        !lua_istable(attenuationState, -1))
        throw std::runtime_error("AudioEmitter did not return its authored curve");
    lua_pushnumber(attenuationState, 150.0);
    lua_gettable(attenuationState, -2);
    if (!lua_isnumber(attenuationState, -1) ||
        lua_tonumber(attenuationState, -1) != 0.0)
        throw std::runtime_error("AudioEmitter changed an authored curve key");
    lua_settop(attenuationState, 0);
    lua_pushnil(attenuationState);
    lua_newtable(attenuationState);
    lua_pushnumber(attenuationState, 5.0);
    lua_pushnumber(attenuationState, 1.25);
    lua_settable(attenuationState, 2);
    bool rejectedInvalidCurve = false;
    try
    {
        audioEmitter->setDistanceAttenuationLua(attenuationState);
    }
    catch (const std::runtime_error&)
    {
        rejectedInvalidCurve = true;
    }
    if (!rejectedInvalidCurve || audioEmitter->distanceAttenuation().size() != 2 ||
        audioEmitter->distanceAttenuation().front().gain != 1.0f)
        throw std::runtime_error(
            "AudioEmitter accepted or partially applied an invalid curve");
    lua_settop(attenuationState, 0);
    lua_pushnil(attenuationState);
    lua_newtable(attenuationState);
    lua_pushnumber(attenuationState, 0.0);
    lua_pushnumber(attenuationState, 1.0);
    lua_settable(attenuationState, 2);
    lua_pushnumber(attenuationState, 180.0);
    lua_pushnumber(attenuationState, 0.0);
    lua_settable(attenuationState, 2);
    audioEmitter->setAngleAttenuationLua(attenuationState);
    lua_settop(attenuationState, 0);
    lua_pushnil(attenuationState);
    lua_newtable(attenuationState);
    lua_pushnumber(attenuationState, 0.0);
    lua_pushnumber(attenuationState, 1.0);
    lua_settable(attenuationState, 2);
    lua_pushnumber(attenuationState, 20.0);
    lua_pushnumber(attenuationState, 0.5);
    lua_settable(attenuationState, 2);
    audioListener->setDistanceAttenuationLua(attenuationState);
    lua_close(attenuationState);
    const float listenerAudibility = audioListener->getAudibilityFor(audioEmitter);
    if (listenerAudibility <= 0.0f || listenerAudibility >= 1.0f ||
        audioEmitter->getAudibilityFor(audioListener) != listenerAudibility ||
        audioEmitter->getInteractingListeners()->size() != 1 ||
        audioListener->getInteractingEmitters()->size() != 1)
        throw std::runtime_error("AudioListener spatial interaction contract failed");
    boost::shared_ptr<RBX::Soundscape::Wire> listenerWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    listenerWire->setName("ListenerWire");
    listenerWire->setSourceInstance(audioListener.get());
    listenerWire->setTargetInstance(audioOutput.get());
    listenerWire->setParent(audioListener.get());
    if (!listenerWire->getConnected())
        throw std::runtime_error("AudioListener did not route to AudioDeviceOutput");
    boost::shared_ptr<RBX::Soundscape::Wire> audioWire =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::Wire>();
    audioWire->setName("DeviceWire");
    unsigned wiringEvents = 0;
    bool observedInputPin = false;
    rbx::signals::scoped_connection wiringConnection =
        audioOutput->wiringChangedSignal.connect(
            [&](bool connected, std::string pin,
                boost::shared_ptr<RBX::Instance> wire,
                boost::shared_ptr<RBX::Instance>) {
                if (connected || wire.get() != audioWire.get())
                    throw std::runtime_error(
                        "invalid sink wire emitted an incorrect WiringChanged payload");
                ++wiringEvents;
                observedInputPin |= pin == "Input";
            });
    audioWire->setTargetInstance(audioOutput.get());
    if (audioWire->getTargetName() != "Input")
        throw std::runtime_error("Wire did not select its target's default pin");
    audioWire->setTargetName("Input");
    audioWire->setParent(audioRoot.get());
    const boost::shared_ptr<const RBX::Reflection::ValueArray> inputPins =
        audioOutput->getInputPinsReflection();
    const boost::shared_ptr<const RBX::Instances> connectedWires =
        audioOutput->getConnectedWiresReflection("Input");
    if (!inputPins || inputPins->size() != 1 ||
        !inputPins->front().isType<std::string>() ||
        inputPins->front().cast<std::string>() != "Input" ||
        !connectedWires || connectedWires->size() != 1 ||
        audioWire->getConnected() || wiringEvents == 0 || !observedInputPin)
        throw std::runtime_error("current audio sink pin/wire contract failed");
    std::stringstream audioEncoded;
    RBX::SerializerBinary::serialize(audioEncoded, audioRoot.get());
    boost::shared_ptr<RBX::Folder> decodedAudioRoot =
        RBX::Creatable<RBX::Instance>::create<RBX::Folder>();
    RBX::SerializerBinary::deserialize(audioEncoded, decodedAudioRoot.get());
    RBX::Soundscape::AudioDeviceOutput* decodedAudioOutput =
        decodedAudioRoot->findFirstChildOfType<
            RBX::Soundscape::AudioDeviceOutput>();
    RBX::Soundscape::Wire* decodedAudioWire =
        RBX::Instance::fastDynamicCast<RBX::Soundscape::Wire>(
            decodedAudioRoot->findFirstChildByNameRecursive("DeviceWire"));
    RBX::Soundscape::AudioEmitter* decodedAudioEmitter =
        decodedAudioRoot->findFirstChildOfType<RBX::Soundscape::AudioEmitter>();
    RBX::Soundscape::AudioFader* decodedAudioFader =
        decodedAudioRoot->findFirstChildOfType<RBX::Soundscape::AudioFader>();
    RBX::Soundscape::AudioDistortion* decodedAudioDistortion =
        decodedAudioRoot->findFirstChildOfType<
            RBX::Soundscape::AudioDistortion>();
    RBX::Soundscape::AudioTremolo* decodedAudioTremolo =
        decodedAudioRoot->findFirstChildOfType<
            RBX::Soundscape::AudioTremolo>();
    RBX::Soundscape::AudioChorus* decodedAudioChorus =
        decodedAudioRoot->findFirstChildOfType<RBX::Soundscape::AudioChorus>();
    RBX::Soundscape::AudioFlanger* decodedAudioFlanger =
        decodedAudioRoot->findFirstChildOfType<RBX::Soundscape::AudioFlanger>();
    RBX::Soundscape::AudioChannelMixer* decodedAudioMixer =
        decodedAudioRoot->findFirstChildOfType<
            RBX::Soundscape::AudioChannelMixer>();
    RBX::Soundscape::AudioChannelSplitter* decodedAudioSplitter =
        decodedAudioRoot->findFirstChildOfType<
            RBX::Soundscape::AudioChannelSplitter>();
    RBX::Soundscape::AudioPlayer* decodedAudioPlayer =
        decodedAudioRoot->findFirstChildOfType<RBX::Soundscape::AudioPlayer>();
    RBX::Soundscape::AudioListener* decodedAudioListener =
        decodedAudioRoot->findFirstChildOfType<RBX::Soundscape::AudioListener>();
    if (!decodedAudioOutput || !decodedAudioWire ||
        !decodedAudioEmitter || !decodedAudioListener || !decodedAudioFader ||
        !decodedAudioDistortion || !decodedAudioTremolo ||
        !decodedAudioChorus || !decodedAudioFlanger ||
        !decodedAudioMixer || !decodedAudioSplitter ||
        decodedAudioMixer->getLayout() != RBX::Soundscape::AUDIO_CHANNEL_QUAD ||
        decodedAudioSplitter->getLayout() != RBX::Soundscape::AUDIO_CHANNEL_QUAD ||
        decodedAudioMixer->getConnectedWiresReflection("Input")->size() != 1 ||
        decodedAudioMixer->getConnectedWiresReflection("Output")->size() != 1 ||
        decodedAudioSplitter->getConnectedWiresReflection("Input")->size() != 1 ||
        decodedAudioSplitter->getConnectedWiresReflection("Output")->size() != 1 ||
        decodedAudioListener->getAudioInteractionGroup() != "World" ||
        decodedAudioListener->getPositionType() !=
            RBX::Soundscape::LISTENER_POSITION_INSTANCE ||
        decodedAudioListener->distanceAttenuation().size() != 2 ||
        decodedAudioFader->getVolume() != 0.25f ||
        decodedAudioFader->getBypass() ||
        decodedAudioDistortion->getLevel() != 0.35f ||
        decodedAudioDistortion->getBypass() ||
        decodedAudioTremolo->getDepth() != 0.7f ||
        decodedAudioTremolo->getDuty() != 0.8f ||
        decodedAudioTremolo->getFrequency() != 6.0f ||
        decodedAudioTremolo->getShape() != 0.25f ||
        decodedAudioTremolo->getSkew() != -0.2f ||
        decodedAudioTremolo->getSquare() != 0.4f ||
        decodedAudioTremolo->getBypass() ||
        decodedAudioTremolo->getConnectedWiresReflection("Input")->size() != 1 ||
        decodedAudioTremolo->getConnectedWiresReflection("Output")->size() != 1 ||
        decodedAudioChorus->getDepth() != 0.6f ||
        decodedAudioChorus->getMix() != 0.3f ||
        decodedAudioChorus->getRate() != 4.0f ||
        decodedAudioChorus->getBypass() ||
        decodedAudioChorus->getConnectedWiresReflection("Input")->size() != 1 ||
        decodedAudioChorus->getConnectedWiresReflection("Output")->size() != 1 ||
        decodedAudioFlanger->getDepth() != 0.4f ||
        decodedAudioFlanger->getMix() != 0.2f ||
        decodedAudioFlanger->getRate() != 3.0f ||
        decodedAudioFlanger->getBypass() ||
        decodedAudioFlanger->getConnectedWiresReflection("Input")->size() != 1 ||
        decodedAudioFlanger->getConnectedWiresReflection("Output")->size() != 1 ||
        !decodedAudioPlayer || decodedAudioPlayer->getAssetId() !=
            "rbxasset://sounds/uuhhh.mp3" ||
        decodedAudioPlayer->getAudioContent().getSourceType() !=
            RBX::CONTENT_SOURCE_URI ||
        decodedAudioPlayer->getAudioContent().getUri() !=
            "rbxassetid://1843529274" ||
        decodedAudioPlayer->getLoopRegion() != RBX::NumberRange(0.1f, 0.3f) ||
        decodedAudioPlayer->getPlaybackRegion() != RBX::NumberRange(0.05f, 0.4f) ||
        decodedAudioEmitter->distanceAttenuation().size() != 2 ||
        decodedAudioEmitter->getPositionType() !=
            RBX::Soundscape::EMITTER_POSITION_INSTANCE ||
        decodedAudioEmitter->distanceAttenuation().back().distance != 150.0f ||
        decodedAudioEmitter->distanceAttenuation().back().gain != 0.0f ||
        decodedAudioEmitter->angleAttenuation().size() != 2 ||
        decodedAudioEmitter->angleAttenuation().back().distance != 180.0f ||
        decodedAudioEmitter->angleAttenuation().back().gain != 0.0f ||
        decodedAudioWire->getTargetInstance() != decodedAudioOutput ||
        decodedAudioWire->getTargetName() != "Input")
        throw std::runtime_error("current audio sink/wire binary round trip failed");

    RBX::DataModel::closeDataModel(dataModel);

    std::cout << "inventory instances=" << binary.instances
              << " parts=" << binary.parts << " scripts=" << binary.scripts
              << " prompts=" << binary.prompts << " fontFaces=" << binary.fontFaces
              << '\n';
    return 0;
}
