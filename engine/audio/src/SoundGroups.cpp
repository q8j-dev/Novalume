#include "audio/SoundGroups.h"
#include "audio/SoundService.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace RBX::Soundscape
{

const char* const sSoundGroup = "SoundGroup";
const char* const sSoundEffect = "SoundEffect";
const char* const sEqualizerSoundEffect = "EqualizerSoundEffect";
const char* const sDistortionSoundEffect = "DistortionSoundEffect";
const char* const sFlangeSoundEffect = "FlangeSoundEffect";
const char* const sEchoSoundEffect = "EchoSoundEffect";
const char* const sTremoloSoundEffect = "TremoloSoundEffect";
const char* const sReverbSoundEffect = "ReverbSoundEffect";
const char* const sPitchShiftSoundEffect = "PitchShiftSoundEffect";
const char* const sChorusSoundEffect = "ChorusSoundEffect";
const char* const sCompressorSoundEffect = "CompressorSoundEffect";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<SoundGroup, float> soundGroupVolume(
    "Volume", category_Data, &SoundGroup::getVolume, &SoundGroup::setVolume);
static Reflection::PropDescriptor<SoundEffect, bool> soundEffectEnabled(
    "Enabled", category_Behavior, &SoundEffect::getEnabled,
    &SoundEffect::setEnabled);
static Reflection::PropDescriptor<SoundEffect, int> soundEffectPriority(
    "Priority", category_Behavior, &SoundEffect::getPriority,
    &SoundEffect::setPriority);

static Reflection::PropDescriptor<EqualizerSoundEffect, float>
    equalizerHighGain("HighGain", category_Data,
        &EqualizerSoundEffect::getHighGain,
        &EqualizerSoundEffect::setHighGain);
static Reflection::PropDescriptor<EqualizerSoundEffect, float>
    equalizerMidGain("MidGain", category_Data,
        &EqualizerSoundEffect::getMidGain,
        &EqualizerSoundEffect::setMidGain);
static Reflection::PropDescriptor<EqualizerSoundEffect, float>
    equalizerLowGain("LowGain", category_Data,
        &EqualizerSoundEffect::getLowGain,
        &EqualizerSoundEffect::setLowGain);
static Reflection::PropDescriptor<DistortionSoundEffect, float>
    distortionLevel("Level", category_Data,
        &DistortionSoundEffect::getLevel,
        &DistortionSoundEffect::setLevel);
static Reflection::PropDescriptor<FlangeSoundEffect, float>
    flangeDepth("Depth", category_Data, &FlangeSoundEffect::getDepth,
        &FlangeSoundEffect::setDepth);
static Reflection::PropDescriptor<FlangeSoundEffect, float>
    flangeMix("Mix", category_Data, &FlangeSoundEffect::getMix,
        &FlangeSoundEffect::setMix);
static Reflection::PropDescriptor<FlangeSoundEffect, float>
    flangeRate("Rate", category_Data, &FlangeSoundEffect::getRate,
        &FlangeSoundEffect::setRate);
static Reflection::PropDescriptor<EchoSoundEffect, float>
    echoDelay("Delay", category_Data, &EchoSoundEffect::getDelay,
        &EchoSoundEffect::setDelay);
static Reflection::PropDescriptor<EchoSoundEffect, float>
    echoDryLevel("DryLevel", category_Data, &EchoSoundEffect::getDryLevel,
        &EchoSoundEffect::setDryLevel);
static Reflection::PropDescriptor<EchoSoundEffect, float>
    echoFeedback("Feedback", category_Data, &EchoSoundEffect::getFeedback,
        &EchoSoundEffect::setFeedback);
static Reflection::PropDescriptor<EchoSoundEffect, float>
    echoWetLevel("WetLevel", category_Data, &EchoSoundEffect::getWetLevel,
        &EchoSoundEffect::setWetLevel);
static Reflection::PropDescriptor<TremoloSoundEffect, float>
    tremoloDepth("Depth", category_Data, &TremoloSoundEffect::getDepth,
        &TremoloSoundEffect::setDepth);
static Reflection::PropDescriptor<TremoloSoundEffect, float>
    tremoloDuty("Duty", category_Data, &TremoloSoundEffect::getDuty,
        &TremoloSoundEffect::setDuty);
static Reflection::PropDescriptor<TremoloSoundEffect, float>
    tremoloFrequency("Frequency", category_Data,
        &TremoloSoundEffect::getFrequency,
        &TremoloSoundEffect::setFrequency);
static Reflection::PropDescriptor<ReverbSoundEffect, float>
    reverbDecayTime("DecayTime", category_Data,
        &ReverbSoundEffect::getDecayTime,
        &ReverbSoundEffect::setDecayTime);
static Reflection::PropDescriptor<ReverbSoundEffect, float>
    reverbDensity("Density", category_Data, &ReverbSoundEffect::getDensity,
        &ReverbSoundEffect::setDensity);
static Reflection::PropDescriptor<ReverbSoundEffect, float>
    reverbDiffusion("Diffusion", category_Data,
        &ReverbSoundEffect::getDiffusion,
        &ReverbSoundEffect::setDiffusion);
static Reflection::PropDescriptor<ReverbSoundEffect, float>
    reverbDryLevel("DryLevel", category_Data,
        &ReverbSoundEffect::getDryLevel,
        &ReverbSoundEffect::setDryLevel);
static Reflection::PropDescriptor<ReverbSoundEffect, float>
    reverbWetLevel("WetLevel", category_Data,
        &ReverbSoundEffect::getWetLevel,
        &ReverbSoundEffect::setWetLevel);
static Reflection::PropDescriptor<PitchShiftSoundEffect, float>
    pitchShiftOctave("Octave", category_Data,
        &PitchShiftSoundEffect::getOctave,
        &PitchShiftSoundEffect::setOctave);
static Reflection::PropDescriptor<ChorusSoundEffect, float>
    chorusDepth("Depth", category_Data, &ChorusSoundEffect::getDepth,
        &ChorusSoundEffect::setDepth);
static Reflection::PropDescriptor<ChorusSoundEffect, float>
    chorusMix("Mix", category_Data, &ChorusSoundEffect::getMix,
        &ChorusSoundEffect::setMix);
static Reflection::PropDescriptor<ChorusSoundEffect, float>
    chorusRate("Rate", category_Data, &ChorusSoundEffect::getRate,
        &ChorusSoundEffect::setRate);
static Reflection::PropDescriptor<CompressorSoundEffect, float>
    compressorAttack("Attack", category_Data,
        &CompressorSoundEffect::getAttack,
        &CompressorSoundEffect::setAttack);
static Reflection::PropDescriptor<CompressorSoundEffect, float>
    compressorGainMakeup("GainMakeup", category_Data,
        &CompressorSoundEffect::getGainMakeup,
        &CompressorSoundEffect::setGainMakeup);
static Reflection::PropDescriptor<CompressorSoundEffect, float>
    compressorRatio("Ratio", category_Data,
        &CompressorSoundEffect::getRatio,
        &CompressorSoundEffect::setRatio);
static Reflection::PropDescriptor<CompressorSoundEffect, float>
    compressorRelease("Release", category_Data,
        &CompressorSoundEffect::getRelease,
        &CompressorSoundEffect::setRelease);
static Reflection::RefPropDescriptor<CompressorSoundEffect, Instance>
    compressorSideChain("SideChain", category_Data,
        &CompressorSoundEffect::getSideChain,
        &CompressorSoundEffect::setSideChain);
static Reflection::PropDescriptor<CompressorSoundEffect, float>
    compressorThreshold("Threshold", category_Data,
        &CompressorSoundEffect::getThreshold,
        &CompressorSoundEffect::setThreshold);
REFLECTION_END();

namespace {

template<typename Class>
void setFiniteClamped(Class* object, float& field, float value,
    float minimum, float maximum,
    const Reflection::PropDescriptor<Class, float>& descriptor,
    const char* propertyName)
{
    if (!std::isfinite(value))
        throw runtime_error("%s must be finite", propertyName);
    value = std::clamp(value, minimum, maximum);
    if (field == value)
        return;
    field = value;
    object->raisePropertyChanged(descriptor);
}

}

SoundGroup::SoundGroup()
    : Super(sSoundGroup)
    , volume(1.0f)
{
}

void SoundGroup::setVolume(float value)
{
    if (!std::isfinite(value))
        throw runtime_error("SoundGroup.Volume must be finite");
    value = std::clamp(value, 0.0f, 10.0f);
    if (volume == value)
        return;
    volume = value;
    if (SoundService* service = ServiceProvider::find<SoundService>(this))
        service->updateSoundGroupVolume(this);
    raisePropertyChanged(soundGroupVolume);
}

SoundEffect::SoundEffect(const char* name)
    : Super(name)
    , enabled(true)
    , priority(0)
{
}

void SoundEffect::setEnabled(bool value)
{
    if (enabled == value)
        return;
    enabled = value;
    raisePropertyChanged(soundEffectEnabled);
}

void SoundEffect::setPriority(int value)
{
    if (priority == value)
        return;
    priority = value;
    raisePropertyChanged(soundEffectPriority);
}

float SoundEffect::effectOwnerKey() const
{
    const std::uintptr_t address = reinterpret_cast<std::uintptr_t>(this);
    return static_cast<float>(address & 0x00ffffffu);
}

std::uint32_t collectSoundEffects(const Instance* parent,
    std::array<Audio::VoiceEffect, 32>& effects)
{
    if (!parent || !parent->getChildren())
        return 0;

    std::vector<const SoundEffect*> ordered;
    const shared_ptr<const Instances> children = parent->getChildren().read();
    if (!children)
        return 0;
    ordered.reserve(children->size());
    for (const shared_ptr<Instance>& child : *children)
    {
        const SoundEffect* effect =
            Instance::fastDynamicCast<SoundEffect>(child.get());
        if (effect && effect->getEnabled())
            ordered.push_back(effect);
    }
    std::stable_sort(ordered.begin(), ordered.end(),
        [](const SoundEffect* left, const SoundEffect* right) {
            return left->getPriority() < right->getPriority();
        });

    const std::size_t count = std::min(ordered.size(), effects.size());
    for (std::size_t index = 0; index < count; ++index)
        ordered[index]->buildVoiceEffect(effects[index]);
    return static_cast<std::uint32_t>(count);
}

EqualizerSoundEffect::EqualizerSoundEffect()
    : Super(sEqualizerSoundEffect)
    , highGain(0.0f)
    , midGain(-10.0f)
    , lowGain(-20.0f)
{
}

void EqualizerSoundEffect::setHighGain(float value)
{
    setFiniteClamped(this, highGain, value, -80.0f, 10.0f,
        equalizerHighGain, "EqualizerSoundEffect.HighGain");
}

void EqualizerSoundEffect::setMidGain(float value)
{
    setFiniteClamped(this, midGain, value, -80.0f, 10.0f,
        equalizerMidGain, "EqualizerSoundEffect.MidGain");
}

void EqualizerSoundEffect::setLowGain(float value)
{
    setFiniteClamped(this, lowGain, value, -80.0f, 10.0f,
        equalizerLowGain, "EqualizerSoundEffect.LowGain");
}

void EqualizerSoundEffect::buildVoiceEffect(Audio::VoiceEffect& effect) const
{
    effect = {};
    effect.type = Audio::VoiceEffectType::Equalizer;
    effect.parameters = {lowGain, midGain, highGain, 400.0f, 4000.0f};
}

DistortionSoundEffect::DistortionSoundEffect()
    : Super(sDistortionSoundEffect)
    , level(0.75f)
{
}

void DistortionSoundEffect::setLevel(float value)
{
    setFiniteClamped(this, level, value, 0.0f, 1.0f, distortionLevel,
        "DistortionSoundEffect.Level");
}

void DistortionSoundEffect::buildVoiceEffect(Audio::VoiceEffect& effect) const
{
    effect = {};
    effect.type = Audio::VoiceEffectType::Distortion;
    effect.parameters[0] = level;
}

FlangeSoundEffect::FlangeSoundEffect()
    : Super(sFlangeSoundEffect)
    , depth(0.45f)
    , mix(0.85f)
    , rate(5.0f)
{
}

void FlangeSoundEffect::setDepth(float value)
{
    setFiniteClamped(this, depth, value, 0.01f, 1.0f, flangeDepth,
        "FlangeSoundEffect.Depth");
}

void FlangeSoundEffect::setMix(float value)
{
    setFiniteClamped(this, mix, value, 0.0f, 1.0f, flangeMix,
        "FlangeSoundEffect.Mix");
}

void FlangeSoundEffect::setRate(float value)
{
    setFiniteClamped(this, rate, value, 0.0f, 20.0f, flangeRate,
        "FlangeSoundEffect.Rate");
}

void FlangeSoundEffect::buildVoiceEffect(Audio::VoiceEffect& effect) const
{
    effect = {};
    effect.type = Audio::VoiceEffectType::Flanger;
    effect.parameters = {depth, mix, rate};
}

EchoSoundEffect::EchoSoundEffect()
    : Super(sEchoSoundEffect)
    , delay(1.0f)
    , dryLevel(0.0f)
    , feedback(0.5f)
    , wetLevel(0.0f)
{
}

void EchoSoundEffect::setDelay(float value)
{
    setFiniteClamped(this, delay, value, 0.1f, 5.0f, echoDelay,
        "EchoSoundEffect.Delay");
}

void EchoSoundEffect::setDryLevel(float value)
{
    setFiniteClamped(this, dryLevel, value, -80.0f, 10.0f, echoDryLevel,
        "EchoSoundEffect.DryLevel");
}

void EchoSoundEffect::setFeedback(float value)
{
    setFiniteClamped(this, feedback, value, 0.0f, 1.0f, echoFeedback,
        "EchoSoundEffect.Feedback");
}

void EchoSoundEffect::setWetLevel(float value)
{
    setFiniteClamped(this, wetLevel, value, -80.0f, 100.0f, echoWetLevel,
        "EchoSoundEffect.WetLevel");
}

void EchoSoundEffect::buildVoiceEffect(Audio::VoiceEffect& effect) const
{
    effect = {};
    effect.type = Audio::VoiceEffectType::Echo;
    effect.parameters = {delay, dryLevel, feedback, 0.0f, wetLevel, 0.0f,
        effectOwnerKey()};
}

TremoloSoundEffect::TremoloSoundEffect()
    : Super(sTremoloSoundEffect)
    , depth(1.0f)
    , duty(0.5f)
    , frequency(5.0f)
{
}

void TremoloSoundEffect::setDepth(float value)
{
    setFiniteClamped(this, depth, value, 0.0f, 1.0f, tremoloDepth,
        "TremoloSoundEffect.Depth");
}

void TremoloSoundEffect::setDuty(float value)
{
    setFiniteClamped(this, duty, value, 0.0f, 1.0f, tremoloDuty,
        "TremoloSoundEffect.Duty");
}

void TremoloSoundEffect::setFrequency(float value)
{
    setFiniteClamped(this, frequency, value, 0.1f, 20.0f,
        tremoloFrequency, "TremoloSoundEffect.Frequency");
}

void TremoloSoundEffect::buildVoiceEffect(Audio::VoiceEffect& effect) const
{
    effect = {};
    effect.type = Audio::VoiceEffectType::Tremolo;
    effect.parameters = {depth, duty, frequency, 0.5f, 0.0f, 0.0f};
}

ReverbSoundEffect::ReverbSoundEffect()
    : Super(sReverbSoundEffect)
    , decayTime(1.5f)
    , density(1.0f)
    , diffusion(1.0f)
    , dryLevel(-6.0f)
    , wetLevel(0.0f)
{
}

void ReverbSoundEffect::setDecayTime(float value)
{
    setFiniteClamped(this, decayTime, value, 0.1f, 20.0f,
        reverbDecayTime, "ReverbSoundEffect.DecayTime");
}

void ReverbSoundEffect::setDensity(float value)
{
    setFiniteClamped(this, density, value, 0.0f, 1.0f, reverbDensity,
        "ReverbSoundEffect.Density");
}

void ReverbSoundEffect::setDiffusion(float value)
{
    setFiniteClamped(this, diffusion, value, 0.0f, 1.0f,
        reverbDiffusion, "ReverbSoundEffect.Diffusion");
}

void ReverbSoundEffect::setDryLevel(float value)
{
    setFiniteClamped(this, dryLevel, value, -80.0f, 20.0f,
        reverbDryLevel, "ReverbSoundEffect.DryLevel");
}

void ReverbSoundEffect::setWetLevel(float value)
{
    setFiniteClamped(this, wetLevel, value, -80.0f, 20.0f,
        reverbWetLevel, "ReverbSoundEffect.WetLevel");
}

void ReverbSoundEffect::buildVoiceEffect(Audio::VoiceEffect& effect) const
{
    effect = {};
    effect.type = Audio::VoiceEffectType::Reverb;
    effect.parameters = {0.5f, decayTime, density, diffusion, dryLevel,
        0.02f, 20000.0f, 0.04f, 250.0f, 0.0f, 5000.0f, wetLevel,
        effectOwnerKey()};
}

PitchShiftSoundEffect::PitchShiftSoundEffect()
    : Super(sPitchShiftSoundEffect)
    , octave(1.25f)
{
}

void PitchShiftSoundEffect::setOctave(float value)
{
    setFiniteClamped(this, octave, value, 0.5f, 2.0f,
        pitchShiftOctave, "PitchShiftSoundEffect.Octave");
}

void PitchShiftSoundEffect::buildVoiceEffect(Audio::VoiceEffect& effect) const
{
    effect = {};
    effect.type = Audio::VoiceEffectType::PitchShifter;
    effect.parameters = {octave, 1.0f};
}

ChorusSoundEffect::ChorusSoundEffect()
    : Super(sChorusSoundEffect)
    , depth(0.15f)
    , mix(0.5f)
    , rate(0.5f)
{
}

void ChorusSoundEffect::setDepth(float value)
{
    setFiniteClamped(this, depth, value, 0.0f, 1.0f, chorusDepth,
        "ChorusSoundEffect.Depth");
}

void ChorusSoundEffect::setMix(float value)
{
    setFiniteClamped(this, mix, value, 0.0f, 1.0f, chorusMix,
        "ChorusSoundEffect.Mix");
}

void ChorusSoundEffect::setRate(float value)
{
    setFiniteClamped(this, rate, value, 0.0f, 20.0f, chorusRate,
        "ChorusSoundEffect.Rate");
}

void ChorusSoundEffect::buildVoiceEffect(Audio::VoiceEffect& effect) const
{
    effect = {};
    effect.type = Audio::VoiceEffectType::Chorus;
    effect.parameters = {depth, mix, rate};
}

CompressorSoundEffect::CompressorSoundEffect()
    : Super(sCompressorSoundEffect)
    , attack(0.1f)
    , gainMakeup(0.0f)
    , ratio(40.0f)
    , release(0.1f)
    , threshold(-40.0f)
{
}

void CompressorSoundEffect::setAttack(float value)
{
    setFiniteClamped(this, attack, value, 0.001f, 1.0f,
        compressorAttack, "CompressorSoundEffect.Attack");
}

void CompressorSoundEffect::setGainMakeup(float value)
{
    setFiniteClamped(this, gainMakeup, value, 0.0f, 30.0f,
        compressorGainMakeup, "CompressorSoundEffect.GainMakeup");
}

void CompressorSoundEffect::setRatio(float value)
{
    setFiniteClamped(this, ratio, value, 1.0f, 50.0f,
        compressorRatio, "CompressorSoundEffect.Ratio");
}

void CompressorSoundEffect::setRelease(float value)
{
    setFiniteClamped(this, release, value, 0.001f, 5.0f,
        compressorRelease, "CompressorSoundEffect.Release");
}

Instance* CompressorSoundEffect::getSideChain() const
{
    return sideChain.lock().get();
}

void CompressorSoundEffect::setSideChain(Instance* value)
{
    if (getSideChain() == value)
        return;
    sideChain = value ? weak_from(value) : weak_ptr<Instance>();
    sideChainMeter.reset();
    raisePropertyChanged(compressorSideChain);
}

void CompressorSoundEffect::setSideChainMeter(
    std::shared_ptr<Audio::MeterState> value)
{
    sideChainMeter = std::move(value);
}

void CompressorSoundEffect::setThreshold(float value)
{
    setFiniteClamped(this, threshold, value, -80.0f, 0.0f,
        compressorThreshold, "CompressorSoundEffect.Threshold");
}

void CompressorSoundEffect::buildVoiceEffect(Audio::VoiceEffect& effect) const
{
    effect = {};
    effect.type = Audio::VoiceEffectType::Compressor;
    effect.parameters = {attack, gainMakeup, ratio, release, threshold};
    effect.meter = sideChainMeter;
}

}
