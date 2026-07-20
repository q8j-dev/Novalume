#pragma once

#include "V8Tree/Instance.h"

namespace RBX::Soundscape
{

extern const char* const sSoundGroup;
extern const char* const sEqualizerSoundEffect;

class SoundGroup final
    : public DescribedCreatable<SoundGroup, Instance, sSoundGroup>
{
public:
    typedef DescribedCreatable<SoundGroup, Instance, sSoundGroup> Super;
    SoundGroup();

    float getVolume() const { return volume; }
    void setVolume(float value);

private:
    float volume;
};

class EqualizerSoundEffect final
    : public DescribedCreatable<EqualizerSoundEffect, Instance,
        sEqualizerSoundEffect>
{
public:
    typedef DescribedCreatable<EqualizerSoundEffect, Instance,
        sEqualizerSoundEffect> Super;
    EqualizerSoundEffect();

    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);
    int getPriority() const { return priority; }
    void setPriority(int value);
    float getHighGain() const { return highGain; }
    void setHighGain(float value);
    float getMidGain() const { return midGain; }
    void setMidGain(float value);
    float getLowGain() const { return lowGain; }
    void setLowGain(float value);

private:
    bool enabled;
    int priority;
    float highGain;
    float midGain;
    float lowGain;
};

} // namespace RBX::Soundscape
