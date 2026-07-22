#pragma once

#include "audio/AudioEngine.h"
#include "V8Tree/Instance.h"

#include <array>
#include <cstdint>

namespace RBX::Soundscape
{

extern const char* const sSoundGroup;
extern const char* const sSoundEffect;
extern const char* const sEqualizerSoundEffect;
extern const char* const sDistortionSoundEffect;
extern const char* const sFlangeSoundEffect;
extern const char* const sEchoSoundEffect;
extern const char* const sTremoloSoundEffect;
extern const char* const sReverbSoundEffect;
extern const char* const sPitchShiftSoundEffect;
extern const char* const sChorusSoundEffect;
extern const char* const sCompressorSoundEffect;

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

class SoundEffect
    : public DescribedNonCreatable<SoundEffect, Instance, sSoundEffect>
{
public:
    typedef DescribedNonCreatable<SoundEffect, Instance, sSoundEffect> Super;

    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);
    int getPriority() const { return priority; }
    void setPriority(int value);

    virtual void buildVoiceEffect(Audio::VoiceEffect& effect) const = 0;

protected:
    explicit SoundEffect(const char* name);
    float effectOwnerKey() const;

private:
    bool enabled;
    int priority;
};

std::uint32_t collectSoundEffects(const Instance* parent,
    std::array<Audio::VoiceEffect, 32>& effects);

class EqualizerSoundEffect final
    : public DescribedCreatable<EqualizerSoundEffect, SoundEffect,
        sEqualizerSoundEffect>
{
public:
    typedef DescribedCreatable<EqualizerSoundEffect, SoundEffect,
        sEqualizerSoundEffect> Super;
    EqualizerSoundEffect();

    float getHighGain() const { return highGain; }
    void setHighGain(float value);
    float getMidGain() const { return midGain; }
    void setMidGain(float value);
    float getLowGain() const { return lowGain; }
    void setLowGain(float value);
    void buildVoiceEffect(Audio::VoiceEffect& effect) const override;

private:
    float highGain;
    float midGain;
    float lowGain;
};

class DistortionSoundEffect final
    : public DescribedCreatable<DistortionSoundEffect, SoundEffect,
        sDistortionSoundEffect>
{
public:
    typedef DescribedCreatable<DistortionSoundEffect, SoundEffect,
        sDistortionSoundEffect> Super;
    DistortionSoundEffect();

    float getLevel() const { return level; }
    void setLevel(float value);
    void buildVoiceEffect(Audio::VoiceEffect& effect) const override;

private:
    float level;
};

class FlangeSoundEffect final
    : public DescribedCreatable<FlangeSoundEffect, SoundEffect,
        sFlangeSoundEffect>
{
public:
    typedef DescribedCreatable<FlangeSoundEffect, SoundEffect,
        sFlangeSoundEffect> Super;
    FlangeSoundEffect();

    float getDepth() const { return depth; }
    void setDepth(float value);
    float getMix() const { return mix; }
    void setMix(float value);
    float getRate() const { return rate; }
    void setRate(float value);
    void buildVoiceEffect(Audio::VoiceEffect& effect) const override;

private:
    float depth;
    float mix;
    float rate;
};

class EchoSoundEffect final
    : public DescribedCreatable<EchoSoundEffect, SoundEffect, sEchoSoundEffect>
{
public:
    typedef DescribedCreatable<EchoSoundEffect, SoundEffect,
        sEchoSoundEffect> Super;
    EchoSoundEffect();

    float getDelay() const { return delay; }
    void setDelay(float value);
    float getDryLevel() const { return dryLevel; }
    void setDryLevel(float value);
    float getFeedback() const { return feedback; }
    void setFeedback(float value);
    float getWetLevel() const { return wetLevel; }
    void setWetLevel(float value);
    void buildVoiceEffect(Audio::VoiceEffect& effect) const override;

private:
    float delay;
    float dryLevel;
    float feedback;
    float wetLevel;
};

class TremoloSoundEffect final
    : public DescribedCreatable<TremoloSoundEffect, SoundEffect,
        sTremoloSoundEffect>
{
public:
    typedef DescribedCreatable<TremoloSoundEffect, SoundEffect,
        sTremoloSoundEffect> Super;
    TremoloSoundEffect();

    float getDepth() const { return depth; }
    void setDepth(float value);
    float getDuty() const { return duty; }
    void setDuty(float value);
    float getFrequency() const { return frequency; }
    void setFrequency(float value);
    void buildVoiceEffect(Audio::VoiceEffect& effect) const override;

private:
    float depth;
    float duty;
    float frequency;
};

class ReverbSoundEffect final
    : public DescribedCreatable<ReverbSoundEffect, SoundEffect,
        sReverbSoundEffect>
{
public:
    typedef DescribedCreatable<ReverbSoundEffect, SoundEffect,
        sReverbSoundEffect> Super;
    ReverbSoundEffect();

    float getDecayTime() const { return decayTime; }
    void setDecayTime(float value);
    float getDensity() const { return density; }
    void setDensity(float value);
    float getDiffusion() const { return diffusion; }
    void setDiffusion(float value);
    float getDryLevel() const { return dryLevel; }
    void setDryLevel(float value);
    float getWetLevel() const { return wetLevel; }
    void setWetLevel(float value);
    void buildVoiceEffect(Audio::VoiceEffect& effect) const override;

private:
    float decayTime;
    float density;
    float diffusion;
    float dryLevel;
    float wetLevel;
};

class PitchShiftSoundEffect final
    : public DescribedCreatable<PitchShiftSoundEffect, SoundEffect,
        sPitchShiftSoundEffect>
{
public:
    typedef DescribedCreatable<PitchShiftSoundEffect, SoundEffect,
        sPitchShiftSoundEffect> Super;
    PitchShiftSoundEffect();

    float getOctave() const { return octave; }
    void setOctave(float value);
    void buildVoiceEffect(Audio::VoiceEffect& effect) const override;

private:
    float octave;
};

class ChorusSoundEffect final
    : public DescribedCreatable<ChorusSoundEffect, SoundEffect,
        sChorusSoundEffect>
{
public:
    typedef DescribedCreatable<ChorusSoundEffect, SoundEffect,
        sChorusSoundEffect> Super;
    ChorusSoundEffect();

    float getDepth() const { return depth; }
    void setDepth(float value);
    float getMix() const { return mix; }
    void setMix(float value);
    float getRate() const { return rate; }
    void setRate(float value);
    void buildVoiceEffect(Audio::VoiceEffect& effect) const override;

private:
    float depth;
    float mix;
    float rate;
};

class CompressorSoundEffect final
    : public DescribedCreatable<CompressorSoundEffect, SoundEffect,
        sCompressorSoundEffect>
{
public:
    typedef DescribedCreatable<CompressorSoundEffect, SoundEffect,
        sCompressorSoundEffect> Super;
    CompressorSoundEffect();

    float getAttack() const { return attack; }
    void setAttack(float value);
    float getGainMakeup() const { return gainMakeup; }
    void setGainMakeup(float value);
    float getRatio() const { return ratio; }
    void setRatio(float value);
    float getRelease() const { return release; }
    void setRelease(float value);
    Instance* getSideChain() const;
    void setSideChain(Instance* value);
    void setSideChainMeter(std::shared_ptr<Audio::MeterState> value);
    float getThreshold() const { return threshold; }
    void setThreshold(float value);
    void buildVoiceEffect(Audio::VoiceEffect& effect) const override;

private:
    float attack;
    float gainMakeup;
    float ratio;
    float release;
    weak_ptr<Instance> sideChain;
    std::shared_ptr<Audio::MeterState> sideChainMeter;
    float threshold;
};

} // namespace RBX::Soundscape
