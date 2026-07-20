#include "audio/SoundGroups.h"

#include <cmath>

namespace RBX::Soundscape
{

const char* const sSoundGroup = "SoundGroup";
const char* const sEqualizerSoundEffect = "EqualizerSoundEffect";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<SoundGroup, float> soundGroupVolume(
    "Volume", category_Data, &SoundGroup::getVolume, &SoundGroup::setVolume);
static Reflection::PropDescriptor<EqualizerSoundEffect, bool> equalizerEnabled(
    "Enabled", category_Behavior, &EqualizerSoundEffect::getEnabled,
    &EqualizerSoundEffect::setEnabled);
static Reflection::PropDescriptor<EqualizerSoundEffect, int> equalizerPriority(
    "Priority", category_Behavior, &EqualizerSoundEffect::getPriority,
    &EqualizerSoundEffect::setPriority);
static Reflection::PropDescriptor<EqualizerSoundEffect, float> equalizerHighGain(
    "HighGain", category_Data, &EqualizerSoundEffect::getHighGain,
    &EqualizerSoundEffect::setHighGain);
static Reflection::PropDescriptor<EqualizerSoundEffect, float> equalizerMidGain(
    "MidGain", category_Data, &EqualizerSoundEffect::getMidGain,
    &EqualizerSoundEffect::setMidGain);
static Reflection::PropDescriptor<EqualizerSoundEffect, float> equalizerLowGain(
    "LowGain", category_Data, &EqualizerSoundEffect::getLowGain,
    &EqualizerSoundEffect::setLowGain);
REFLECTION_END();

SoundGroup::SoundGroup()
    : Super(sSoundGroup)
    , volume(1.0f)
{
}

void SoundGroup::setVolume(float value)
{
    if (!std::isfinite(value))
        throw runtime_error("SoundGroup.Volume must be finite");
    if (volume == value)
        return;
    volume = value;
    raisePropertyChanged(soundGroupVolume);
}

EqualizerSoundEffect::EqualizerSoundEffect()
    : Super(sEqualizerSoundEffect)
    , enabled(true)
    , priority(0)
    , highGain(0.0f)
    , midGain(0.0f)
    , lowGain(0.0f)
{
}

void EqualizerSoundEffect::setEnabled(bool value)
{
    if (enabled == value)
        return;
    enabled = value;
    raisePropertyChanged(equalizerEnabled);
}

void EqualizerSoundEffect::setPriority(int value)
{
    if (priority == value)
        return;
    priority = value;
    raisePropertyChanged(equalizerPriority);
}

namespace {
void validateGain(float value)
{
    if (!std::isfinite(value))
        throw runtime_error("EqualizerSoundEffect gain must be finite");
}
}

void EqualizerSoundEffect::setHighGain(float value)
{
    validateGain(value);
    if (highGain == value)
        return;
    highGain = value;
    raisePropertyChanged(equalizerHighGain);
}

void EqualizerSoundEffect::setMidGain(float value)
{
    validateGain(value);
    if (midGain == value)
        return;
    midGain = value;
    raisePropertyChanged(equalizerMidGain);
}

void EqualizerSoundEffect::setLowGain(float value)
{
    validateGain(value);
    if (lowGain == value)
        return;
    lowGain = value;
    raisePropertyChanged(equalizerLowGain);
}

} // namespace RBX::Soundscape
