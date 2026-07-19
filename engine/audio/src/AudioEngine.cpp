#include "audio/AudioEngine.h"
#include "FfmpegAudioDecoder.h"

#include <miniaudio.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <utility>

namespace RBX::Audio {

namespace {

void requireSuccess(ma_result result, const char* operation)
{
    if (result != MA_SUCCESS)
        throw std::runtime_error(operation);
}

} // namespace

struct Engine::Impl
{
    struct EffectNode
    {
        struct LongDelay
        {
            std::vector<float> samples;
            std::uint32_t resetMarker = 0;
            std::uint32_t ownerKey = 0;
        };
        ma_node_base base{};
        std::array<std::atomic<std::uint8_t>, 32> types{};
        std::array<std::array<std::atomic<float>, 16>, 32> parameters{};
        std::array<float, 32> phases{};
        std::array<float, 32> rampTargets{};
        std::array<float, 32> rampSteps{};
        std::array<std::uint32_t, 32> rampRemaining{};
        std::array<std::vector<float>, 32> delays;
        std::array<std::vector<float>, 32> states;
        std::array<std::vector<float>, 32> secondaryStates;
        std::array<std::vector<float>, 32> tertiaryStates;
        std::array<std::uint32_t, 32> delayPositions{};
        std::array<std::atomic<LongDelay*>, 32> longDelays{};
        std::vector<std::unique_ptr<LongDelay>> retainedLongDelays;
        std::array<std::atomic<MeterState*>, 32> meters{};
        std::vector<std::shared_ptr<MeterState>> retainedMeters;
        std::array<std::uint32_t, 32> analysisFilled{};
        std::array<std::uint32_t, 32> analysisSinceSpectrum{};
        std::uint32_t sampleRate = 48000;
        std::uint32_t channels = 2;
        std::atomic<std::uint32_t> count{0};
    };

    static void processEffectsNode(ma_node* node, const float** input,
        ma_uint32* inputFrames, float** output, ma_uint32* outputFrames)
    {
        EffectNode* effects = static_cast<EffectNode*>(node);
        const ma_uint32 frames = std::min(*inputFrames, *outputFrames);
        const ma_uint32 channels = effects->base.pInputBuses[0].channels;
        const std::size_t samples = static_cast<std::size_t>(frames) * channels;
        if (!input[0])
        {
            std::fill_n(output[0], samples, 0.0f);
        }
        const std::uint32_t effectCount = std::min<std::uint32_t>(
            effects->count.load(std::memory_order_acquire),
            static_cast<std::uint32_t>(effects->types.size()));
        if (input[0] && effectCount == 0)
        {
            std::copy_n(input[0], samples, output[0]);
        }
        else if (input[0])
        {
            std::copy_n(input[0], samples, output[0]);
            for (std::uint32_t effect = 0; effect < effectCount; ++effect)
            {
                const VoiceEffectType type = static_cast<VoiceEffectType>(
                    effects->types[effect].load(std::memory_order_relaxed));
                if (type == VoiceEffectType::Distortion)
                {
                    const float level = std::clamp(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed), 0.0f, 1.0f);
                    if (level == 0.0f)
                        continue;
                    const float drive = 1.0f + level * 24.0f;
                    const float normalization = std::tanh(drive);
                    for (std::size_t sample = 0; sample < samples; ++sample)
                        output[0][sample] = std::tanh(
                            output[0][sample] * drive) / normalization;
                }
                else if (type == VoiceEffectType::Tremolo)
                {
                    const float depth = std::clamp(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed), 0.0f, 1.0f);
                    const float duty = std::clamp(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed), 0.0f, 1.0f);
                    const float frequency = std::clamp(
                        effects->parameters[effect][2].load(
                            std::memory_order_relaxed), 0.0f, 20.0f);
                    const float shape = std::clamp(
                        effects->parameters[effect][3].load(
                            std::memory_order_relaxed), 0.0f, 1.0f);
                    const float skew = std::clamp(
                        effects->parameters[effect][4].load(
                            std::memory_order_relaxed), -1.0f, 1.0f);
                    const float square = std::clamp(
                        effects->parameters[effect][5].load(
                            std::memory_order_relaxed), 0.0f, 1.0f);
                    float phase = effects->phases[effect];
                    const float phaseStep = frequency /
                        static_cast<float>(std::max(effects->sampleRate, 1u));
                    const float peak = std::clamp(0.5f + skew * 0.49f,
                        0.01f, 0.99f);
                    const float curvePower = std::exp2((shape - 0.5f) * 4.0f);
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                    {
                        float lfo = 0.0f;
                        if (duty > 0.0f && phase < duty)
                        {
                            const float activePhase = phase / duty;
                            const float warped = activePhase < peak
                                ? 0.5f * activePhase / peak
                                : 0.5f + 0.5f * (activePhase - peak) /
                                    (1.0f - peak);
                            const float rounded = 0.5f - 0.5f * std::cos(
                                6.2831853071795864769f * warped);
                            const float shaped = std::pow(
                                std::max(rounded, 0.0f), curvePower);
                            const float pulse = rounded >= 0.5f ? 1.0f : 0.0f;
                            lfo = shaped + (pulse - shaped) * square;
                        }
                        const float gain = 1.0f - depth * lfo;
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                            output[0][static_cast<std::size_t>(frame) * channels +
                                channel] *= gain;
                        phase += phaseStep;
                        phase -= std::floor(phase);
                    }
                    effects->phases[effect] = phase;
                }
                else if (type == VoiceEffectType::Chorus ||
                         type == VoiceEffectType::Flanger)
                {
                    const float depth = std::clamp(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed), 0.0f, 1.0f);
                    const float mix = std::clamp(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed), 0.0f, 1.0f);
                    const float rate = std::clamp(
                        effects->parameters[effect][2].load(
                            std::memory_order_relaxed), 0.0f, 20.0f);
                    std::vector<float>& delay = effects->delays[effect];
                    const std::uint32_t delayFrames = channels && !delay.empty()
                        ? static_cast<std::uint32_t>(delay.size() / channels) : 0;
                    if (delayFrames < 2 || mix == 0.0f)
                        continue;
                    std::uint32_t writeFrame = effects->delayPositions[effect] %
                        delayFrames;
                    float phase = effects->phases[effect];
                    const float phaseStep = rate /
                        static_cast<float>(std::max(effects->sampleRate, 1u));
                    const float baseDelaySeconds = type == VoiceEffectType::Chorus
                        ? 0.018f : 0.0015f;
                    const float depthSeconds = type == VoiceEffectType::Chorus
                        ? 0.012f : 0.00125f;
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                    {
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                        {
                            const std::size_t outputIndex =
                                static_cast<std::size_t>(frame) * channels + channel;
                            const float dry = output[0][outputIndex];
                            delay[static_cast<std::size_t>(writeFrame) * channels +
                                channel] = dry;
                            const float channelPhase = phase +
                                (type == VoiceEffectType::Chorus && channels > 1
                                    ? 0.25f * static_cast<float>(channel) /
                                        static_cast<float>(channels - 1)
                                    : 0.0f);
                            const float lfo = 0.5f + 0.5f * std::sin(
                                6.2831853071795864769f * channelPhase);
                            const float delayInFrames = (baseDelaySeconds +
                                depthSeconds * depth * lfo) * effects->sampleRate;
                            float readFrame = static_cast<float>(writeFrame) -
                                delayInFrames;
                            while (readFrame < 0.0f)
                                readFrame += static_cast<float>(delayFrames);
                            const std::uint32_t first =
                                static_cast<std::uint32_t>(readFrame) % delayFrames;
                            const std::uint32_t second = (first + 1) % delayFrames;
                            const float fraction = readFrame - std::floor(readFrame);
                            const float a = delay[static_cast<std::size_t>(first) *
                                channels + channel];
                            const float b = delay[static_cast<std::size_t>(second) *
                                channels + channel];
                            const float wet = a + (b - a) * fraction;
                            output[0][outputIndex] = dry + (wet - dry) * mix;
                        }
                        writeFrame = (writeFrame + 1) % delayFrames;
                        phase += phaseStep;
                        phase -= std::floor(phase);
                    }
                    effects->delayPositions[effect] = writeFrame;
                    effects->phases[effect] = phase;
                }
                else if (type == VoiceEffectType::Compressor)
                {
                    const float attack = std::clamp(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed), 0.0001f, 0.5f);
                    const float makeupGain = std::clamp(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed), -30.0f, 30.0f);
                    const float ratio = std::clamp(
                        effects->parameters[effect][2].load(
                            std::memory_order_relaxed), 1.0f, 50.0f);
                    const float release = std::clamp(
                        effects->parameters[effect][3].load(
                            std::memory_order_relaxed), 0.01f, 5.0f);
                    const float threshold = std::clamp(
                        effects->parameters[effect][4].load(
                            std::memory_order_relaxed), -60.0f, 0.0f);
                    std::vector<float>& envelopes = effects->states[effect];
                    if (envelopes.size() < channels)
                        continue;
                    const float attackCoefficient = std::exp(-1.0f /
                        (attack * effects->sampleRate));
                    const float releaseCoefficient = std::exp(-1.0f /
                        (release * effects->sampleRate));
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                        {
                            const std::size_t index =
                                static_cast<std::size_t>(frame) * channels + channel;
                            const float level = std::abs(output[0][index]);
                            const float coefficient = level > envelopes[channel]
                                ? attackCoefficient : releaseCoefficient;
                            envelopes[channel] = coefficient * envelopes[channel] +
                                (1.0f - coefficient) * level;
                            const float levelDb = 20.0f * std::log10(
                                std::max(envelopes[channel], 0.0000001f));
                            float gainDb = makeupGain;
                            if (levelDb > threshold)
                                gainDb += threshold + (levelDb - threshold) /
                                    ratio - levelDb;
                            output[0][index] *= std::pow(10.0f, gainDb / 20.0f);
                        }
                }
                else if (type == VoiceEffectType::Gate)
                {
                    const float attack = std::clamp(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed), 0.001f, 5.0f);
                    const float release = std::clamp(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed), 0.001f, 5.0f);
                    const float closeThreshold = std::clamp(
                        effects->parameters[effect][2].load(
                            std::memory_order_relaxed), -80.0f, 30.0f);
                    const float openThreshold = std::clamp(
                        effects->parameters[effect][3].load(
                            std::memory_order_relaxed), closeThreshold, 30.0f);
                    std::vector<float>& gains = effects->states[effect];
                    std::vector<float>& openStates =
                        effects->secondaryStates[effect];
                    if (gains.size() < channels || openStates.size() < channels)
                        continue;
                    const float attackCoefficient = std::exp(-1.0f /
                        (attack * effects->sampleRate));
                    const float releaseCoefficient = std::exp(-1.0f /
                        (release * effects->sampleRate));
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                        {
                            const std::size_t index =
                                static_cast<std::size_t>(frame) * channels + channel;
                            const float levelDb = 20.0f * std::log10(std::max(
                                std::abs(output[0][index]), 0.0000001f));
                            bool open = openStates[channel] > 0.5f;
                            if (open && levelDb < closeThreshold)
                                open = false;
                            else if (!open && levelDb > openThreshold)
                                open = true;
                            openStates[channel] = open ? 1.0f : 0.0f;
                            const float target = open ? 1.0f : 0.0f;
                            const float coefficient = open
                                ? attackCoefficient : releaseCoefficient;
                            gains[channel] = coefficient * gains[channel] +
                                (1.0f - coefficient) * target;
                            output[0][index] *= gains[channel];
                        }
                }
                else if (type == VoiceEffectType::Limiter)
                {
                    const float ceiling = std::pow(10.0f, std::clamp(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed), -12.0f, 0.0f) / 20.0f);
                    const float release = std::clamp(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed), 0.001f, 1.0f);
                    std::vector<float>& gains =
                        effects->secondaryStates[effect];
                    if (gains.size() < channels)
                        continue;
                    for (ma_uint32 channel = 0; channel < channels; ++channel)
                        if (gains[channel] <= 0.0f)
                            gains[channel] = 1.0f;
                    const float releaseCoefficient = std::exp(-1.0f /
                        (release * effects->sampleRate));
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                        {
                            const std::size_t index =
                                static_cast<std::size_t>(frame) * channels + channel;
                            const float level = std::abs(output[0][index]);
                            const float desired = level > ceiling
                                ? ceiling / level : 1.0f;
                            if (desired < gains[channel])
                                gains[channel] = desired;
                            else
                                gains[channel] = releaseCoefficient *
                                    gains[channel] +
                                    (1.0f - releaseCoefficient);
                            output[0][index] *= gains[channel];
                        }
                }
                else if (type == VoiceEffectType::Equalizer)
                {
                    const float lowGain = std::pow(10.0f, std::clamp(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed), -80.0f, 10.0f) / 20.0f);
                    const float midGain = std::pow(10.0f, std::clamp(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed), -80.0f, 10.0f) / 20.0f);
                    const float highGain = std::pow(10.0f, std::clamp(
                        effects->parameters[effect][2].load(
                            std::memory_order_relaxed), -80.0f, 10.0f) / 20.0f);
                    const float lowFrequency = std::clamp(
                        effects->parameters[effect][3].load(
                            std::memory_order_relaxed), 200.0f, 20000.0f);
                    const float highFrequency = std::clamp(
                        effects->parameters[effect][4].load(
                            std::memory_order_relaxed), lowFrequency, 20000.0f);
                    std::vector<float>& lowStates = effects->states[effect];
                    std::vector<float>& highStates =
                        effects->secondaryStates[effect];
                    if (lowStates.size() < channels || highStates.size() < channels)
                        continue;
                    const float lowAlpha = 1.0f - std::exp(
                        -6.2831853071795864769f * lowFrequency /
                        effects->sampleRate);
                    const float highAlpha = 1.0f - std::exp(
                        -6.2831853071795864769f * highFrequency /
                        effects->sampleRate);
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                        {
                            const std::size_t index =
                                static_cast<std::size_t>(frame) * channels + channel;
                            const float inputSample = output[0][index];
                            lowStates[channel] += lowAlpha *
                                (inputSample - lowStates[channel]);
                            highStates[channel] += highAlpha *
                                (inputSample - highStates[channel]);
                            const float low = lowStates[channel];
                            const float mid = highStates[channel] - low;
                            const float high = inputSample - highStates[channel];
                            output[0][index] = low * lowGain + mid * midGain +
                                high * highGain;
                        }
                }
                else if (type == VoiceEffectType::Filter)
                {
                    const int filterType = std::clamp(static_cast<int>(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed)), 0, 11);
                    const float sampleRate = static_cast<float>(
                        std::max(effects->sampleRate, 1u));
                    const float frequency = std::clamp(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed), 20.0f,
                        sampleRate * 0.49f);
                    const float gain = std::clamp(
                        effects->parameters[effect][2].load(
                            std::memory_order_relaxed), -30.0f, 30.0f);
                    const float q = std::clamp(
                        effects->parameters[effect][3].load(
                            std::memory_order_relaxed), 0.1f, 10.0f);
                    if (filterType == 11)
                    {
                        std::vector<float>& filterStates =
                            effects->states[effect];
                        if (filterStates.size() < channels)
                            continue;
                        const float alpha = 1.0f - std::exp(
                            -6.2831853071795864769f * frequency / sampleRate);
                        for (ma_uint32 frame = 0; frame < frames; ++frame)
                            for (ma_uint32 channel = 0; channel < channels; ++channel)
                            {
                                const std::size_t index =
                                    static_cast<std::size_t>(frame) * channels + channel;
                                filterStates[channel] += alpha *
                                    (output[0][index] - filterStates[channel]);
                                output[0][index] = filterStates[channel];
                            }
                        continue;
                    }
                    const float omega = 6.2831853071795864769f * frequency /
                        sampleRate;
                    const float cosine = std::cos(omega);
                    const float sine = std::sin(omega);
                    const float alpha = sine / (2.0f * q);
                    const float amplitude = std::pow(10.0f, gain / 40.0f);
                    float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
                    float a0 = 1.0f, a1 = 0.0f, a2 = 0.0f;
                    if (filterType == 0)
                    {
                        b0 = 1.0f + alpha * amplitude; b1 = -2.0f * cosine;
                        b2 = 1.0f - alpha * amplitude;
                        a0 = 1.0f + alpha / amplitude; a1 = -2.0f * cosine;
                        a2 = 1.0f - alpha / amplitude;
                    }
                    else if (filterType == 1 || filterType == 2)
                    {
                        const float root = 2.0f * std::sqrt(amplitude) * alpha;
                        if (filterType == 1)
                        {
                            b0 = amplitude * ((amplitude + 1.0f) -
                                (amplitude - 1.0f) * cosine + root);
                            b1 = 2.0f * amplitude * ((amplitude - 1.0f) -
                                (amplitude + 1.0f) * cosine);
                            b2 = amplitude * ((amplitude + 1.0f) -
                                (amplitude - 1.0f) * cosine - root);
                            a0 = (amplitude + 1.0f) +
                                (amplitude - 1.0f) * cosine + root;
                            a1 = -2.0f * ((amplitude - 1.0f) +
                                (amplitude + 1.0f) * cosine);
                            a2 = (amplitude + 1.0f) +
                                (amplitude - 1.0f) * cosine - root;
                        }
                        else
                        {
                            b0 = amplitude * ((amplitude + 1.0f) +
                                (amplitude - 1.0f) * cosine + root);
                            b1 = -2.0f * amplitude * ((amplitude - 1.0f) +
                                (amplitude + 1.0f) * cosine);
                            b2 = amplitude * ((amplitude + 1.0f) +
                                (amplitude - 1.0f) * cosine - root);
                            a0 = (amplitude + 1.0f) -
                                (amplitude - 1.0f) * cosine + root;
                            a1 = 2.0f * ((amplitude - 1.0f) -
                                (amplitude + 1.0f) * cosine);
                            a2 = (amplitude + 1.0f) -
                                (amplitude - 1.0f) * cosine - root;
                        }
                    }
                    else if (filterType >= 6 && filterType <= 8)
                    {
                        b0 = (1.0f + cosine) * 0.5f; b1 = -(1.0f + cosine);
                        b2 = b0; a0 = 1.0f + alpha; a1 = -2.0f * cosine;
                        a2 = 1.0f - alpha;
                    }
                    else if (filterType == 9)
                    {
                        b0 = alpha; b1 = 0.0f; b2 = -alpha;
                        a0 = 1.0f + alpha; a1 = -2.0f * cosine;
                        a2 = 1.0f - alpha;
                    }
                    else if (filterType == 10)
                    {
                        b0 = 1.0f; b1 = -2.0f * cosine; b2 = 1.0f;
                        a0 = 1.0f + alpha; a1 = -2.0f * cosine;
                        a2 = 1.0f - alpha;
                    }
                    else
                    {
                        b0 = (1.0f - cosine) * 0.5f; b1 = 1.0f - cosine;
                        b2 = b0; a0 = 1.0f + alpha; a1 = -2.0f * cosine;
                        a2 = 1.0f - alpha;
                    }
                    b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
                    const int stages = (filterType == 4 || filterType == 7) ? 2 :
                        (filterType == 5 || filterType == 8) ? 4 : 1;
                    std::vector<float>& filterStates = effects->delays[effect];
                    if (filterStates.size() < static_cast<std::size_t>(
                            stages * 2) * channels)
                        continue;
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                        {
                            const std::size_t index =
                                static_cast<std::size_t>(frame) * channels + channel;
                            float sample = output[0][index];
                            for (int stage = 0; stage < stages; ++stage)
                            {
                                const std::size_t stateOffset =
                                    static_cast<std::size_t>(stage * 2) * channels;
                                float& z1 = filterStates[stateOffset + channel];
                                float& z2 = filterStates[stateOffset + channels + channel];
                                const float result = b0 * sample + z1;
                                z1 = b1 * sample - a1 * result + z2;
                                z2 = b2 * sample - a2 * result;
                                sample = result;
                            }
                            output[0][index] = sample;
                        }
                }
                else if (type == VoiceEffectType::PitchShifter)
                {
                    const float pitch = std::clamp(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed), 0.5f, 2.0f);
                    if (std::abs(pitch - 1.0f) < 0.00001f)
                        continue;
                    const int windowSelection = std::clamp(static_cast<int>(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed)), 0, 2);
                    const std::uint32_t windowFrames =
                        windowSelection == 0 ? 512u :
                        windowSelection == 1 ? 1024u : 2048u;
                    std::vector<float>& delay = effects->delays[effect];
                    const std::uint32_t delayFrames = channels && !delay.empty()
                        ? static_cast<std::uint32_t>(delay.size() / channels) : 0;
                    if (delayFrames < windowFrames + 2)
                        continue;
                    std::uint32_t writeFrame = effects->delayPositions[effect] %
                        delayFrames;
                    float phase = effects->phases[effect];
                    const float phaseStep = (1.0f - pitch) /
                        static_cast<float>(windowFrames);
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                    {
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                        {
                            const std::size_t outputIndex =
                                static_cast<std::size_t>(frame) * channels + channel;
                            delay[static_cast<std::size_t>(writeFrame) * channels +
                                channel] = output[0][outputIndex];
                            float shifted = 0.0f;
                            for (int head = 0; head < 2; ++head)
                            {
                                float headPhase = phase + 0.5f * head;
                                headPhase -= std::floor(headPhase);
                                float readFrame = static_cast<float>(writeFrame) -
                                    headPhase * windowFrames;
                                while (readFrame < 0.0f)
                                    readFrame += static_cast<float>(delayFrames);
                                const std::uint32_t first =
                                    static_cast<std::uint32_t>(readFrame) % delayFrames;
                                const std::uint32_t second = (first + 1) % delayFrames;
                                const float fraction = readFrame - std::floor(readFrame);
                                const float a = delay[static_cast<std::size_t>(first) *
                                    channels + channel];
                                const float b = delay[static_cast<std::size_t>(second) *
                                    channels + channel];
                                const float weight = 0.5f - 0.5f * std::cos(
                                    6.2831853071795864769f * headPhase);
                                shifted += (a + (b - a) * fraction) * weight;
                            }
                            output[0][outputIndex] = shifted;
                        }
                        writeFrame = (writeFrame + 1) % delayFrames;
                        phase += phaseStep;
                        phase -= std::floor(phase);
                    }
                    effects->delayPositions[effect] = writeFrame;
                    effects->phases[effect] = phase;
                }
                else if (type == VoiceEffectType::Echo)
                {
                    EffectNode::LongDelay* longDelay =
                        effects->longDelays[effect].load(std::memory_order_acquire);
                    if (!longDelay || longDelay->samples.empty())
                        continue;
                    std::vector<float>& delay = longDelay->samples;
                    const std::uint32_t delayFrames = channels
                        ? static_cast<std::uint32_t>(delay.size() / channels) : 0;
                    if (delayFrames < 2)
                        continue;
                    const float targetDelay = std::clamp(
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed), 0.001f, 5.0f) *
                        effects->sampleRate;
                    const float dryGain = std::pow(10.0f, std::clamp(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed), -80.0f, 10.0f) / 20.0f);
                    const float feedback = std::clamp(
                        effects->parameters[effect][2].load(
                            std::memory_order_relaxed), 0.0f, 1.0f);
                    const float rampTime = std::clamp(
                        effects->parameters[effect][3].load(
                            std::memory_order_relaxed), 0.0f, 60000.0f);
                    const float wetGain = std::pow(10.0f, std::clamp(
                        effects->parameters[effect][4].load(
                            std::memory_order_relaxed), -80.0f, 10.0f) / 20.0f);
                    float currentDelay = effects->phases[effect];
                    if (currentDelay < 1.0f || rampTime == 0.0f)
                    {
                        currentDelay = targetDelay;
                        effects->rampTargets[effect] = targetDelay;
                        effects->rampRemaining[effect] = 0;
                    }
                    else if (effects->rampTargets[effect] != targetDelay)
                    {
                        const std::uint32_t rampFrames = static_cast<std::uint32_t>(
                            std::clamp(rampTime * effects->sampleRate, 1.0f,
                                static_cast<float>(UINT32_MAX)));
                        effects->rampTargets[effect] = targetDelay;
                        effects->rampSteps[effect] =
                            (targetDelay - currentDelay) / rampFrames;
                        effects->rampRemaining[effect] = rampFrames;
                    }
                    std::uint32_t writeFrame = effects->delayPositions[effect] %
                        delayFrames;
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                    {
                        if (effects->rampRemaining[effect] > 0)
                        {
                            currentDelay += effects->rampSteps[effect];
                            if (--effects->rampRemaining[effect] == 0)
                                currentDelay = targetDelay;
                        }
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                        {
                            const std::size_t outputIndex =
                                static_cast<std::size_t>(frame) * channels + channel;
                            const float dry = output[0][outputIndex];
                            float readFrame = static_cast<float>(writeFrame) -
                                currentDelay;
                            while (readFrame < 0.0f)
                                readFrame += static_cast<float>(delayFrames);
                            const std::uint32_t first =
                                static_cast<std::uint32_t>(readFrame) % delayFrames;
                            const std::uint32_t second = (first + 1) % delayFrames;
                            const float fraction = readFrame - std::floor(readFrame);
                            const float a = delay[static_cast<std::size_t>(first) *
                                channels + channel];
                            const float b = delay[static_cast<std::size_t>(second) *
                                channels + channel];
                            const float wet = a + (b - a) * fraction;
                            delay[static_cast<std::size_t>(writeFrame) * channels +
                                channel] = dry + wet * feedback;
                            output[0][outputIndex] = dry * dryGain + wet * wetGain;
                        }
                        writeFrame = (writeFrame + 1) % delayFrames;
                    }
                    effects->delayPositions[effect] = writeFrame;
                    effects->phases[effect] = currentDelay;
                }
                else if (type == VoiceEffectType::Reverb)
                {
                    EffectNode::LongDelay* longDelay =
                        effects->longDelays[effect].load(std::memory_order_acquire);
                    if (!longDelay || longDelay->samples.empty())
                        continue;
                    std::vector<float>& delay = longDelay->samples;
                    const std::uint32_t delayFrames = channels
                        ? static_cast<std::uint32_t>(delay.size() / channels) : 0;
                    if (delayFrames < 2)
                        continue;
                    const float decayRatio = std::clamp(
                        effects->parameters[effect][0].load(std::memory_order_relaxed),
                        0.1f, 1.0f);
                    const float decayTime = std::clamp(
                        effects->parameters[effect][1].load(std::memory_order_relaxed),
                        0.1f, 20.0f);
                    const float density = std::clamp(
                        effects->parameters[effect][2].load(std::memory_order_relaxed),
                        0.1f, 1.0f);
                    const float diffusion = std::clamp(
                        effects->parameters[effect][3].load(std::memory_order_relaxed),
                        0.1f, 1.0f);
                    const float dryGain = std::pow(10.0f, std::clamp(
                        effects->parameters[effect][4].load(std::memory_order_relaxed),
                        -80.0f, 20.0f) / 20.0f);
                    const float early = std::clamp(
                        effects->parameters[effect][5].load(std::memory_order_relaxed),
                        0.0f, 0.3f);
                    const float highCut = std::clamp(
                        effects->parameters[effect][6].load(std::memory_order_relaxed),
                        20.0f, 20000.0f);
                    const float late = std::clamp(
                        effects->parameters[effect][7].load(std::memory_order_relaxed),
                        0.0f, 0.1f);
                    const float lowShelfFrequency = std::clamp(
                        effects->parameters[effect][8].load(std::memory_order_relaxed),
                        20.0f, 20000.0f);
                    const float lowShelfGain = std::pow(10.0f, std::clamp(
                        effects->parameters[effect][9].load(std::memory_order_relaxed),
                        -36.0f, 12.0f) / 20.0f);
                    const float referenceFrequency = std::clamp(
                        effects->parameters[effect][10].load(std::memory_order_relaxed),
                        20.0f, 20000.0f);
                    const float wetGain = std::pow(10.0f, std::clamp(
                        effects->parameters[effect][11].load(std::memory_order_relaxed),
                        -80.0f, 20.0f) / 20.0f);
                    const std::array<float, 4> tapSeconds = {
                        early + late,
                        early + late + 0.0113f * density,
                        early + late + 0.0171f * density,
                        early + late + 0.0297f * density};
                    const float maximumTap = *std::max_element(
                        tapSeconds.begin(), tapSeconds.end());
                    const float feedback = std::pow(0.001f,
                        maximumTap / decayTime);
                    const float highAlpha = 1.0f - std::exp(
                        -6.2831853071795864769f * highCut / effects->sampleRate);
                    const float lowAlpha = 1.0f - std::exp(
                        -6.2831853071795864769f * lowShelfFrequency /
                        effects->sampleRate);
                    const float referenceAlpha = 1.0f - std::exp(
                        -6.2831853071795864769f * referenceFrequency /
                        effects->sampleRate);
                    std::vector<float>& highStates = effects->states[effect];
                    std::vector<float>& lowStates = effects->secondaryStates[effect];
                    std::vector<float>& referenceStates =
                        effects->tertiaryStates[effect];
                    if (highStates.size() < channels || lowStates.size() < channels ||
                        referenceStates.size() < channels)
                        continue;
                    std::uint32_t writeFrame = effects->delayPositions[effect] %
                        delayFrames;
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                    {
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                        {
                            const std::size_t outputIndex =
                                static_cast<std::size_t>(frame) * channels + channel;
                            const float dry = output[0][outputIndex];
                            float firstTap = 0.0f;
                            float diffuse = 0.0f;
                            for (std::size_t tap = 0; tap < tapSeconds.size(); ++tap)
                            {
                                const std::uint32_t offset = static_cast<std::uint32_t>(
                                    tapSeconds[tap] * effects->sampleRate) % delayFrames;
                                const std::uint32_t readFrame =
                                    (writeFrame + delayFrames - offset) % delayFrames;
                                const float value = delay[static_cast<std::size_t>(
                                    readFrame) * channels + channel];
                                if (tap == 0) firstTap = value;
                                diffuse += value;
                            }
                            diffuse *= 0.25f;
                            float wet = firstTap + (diffuse - firstTap) * diffusion;
                            highStates[channel] += highAlpha *
                                (wet - highStates[channel]);
                            wet = highStates[channel];
                            lowStates[channel] += lowAlpha *
                                (wet - lowStates[channel]);
                            wet += lowStates[channel] * (lowShelfGain - 1.0f);
                            referenceStates[channel] += referenceAlpha *
                                (wet - referenceStates[channel]);
                            const float highDecay = wet - referenceStates[channel];
                            const float feedbackWet = wet - highDecay *
                                (1.0f - decayRatio);
                            delay[static_cast<std::size_t>(writeFrame) * channels +
                                channel] = dry + feedbackWet * feedback;
                            output[0][outputIndex] = dry * dryGain + wet * wetGain;
                        }
                        writeFrame = (writeFrame + 1) % delayFrames;
                    }
                    effects->delayPositions[effect] = writeFrame;
                }
                else if (type == VoiceEffectType::Analyzer)
                {
                    MeterState* meter = effects->meters[effect].load(
                        std::memory_order_acquire);
                    if (!meter)
                        continue;
                    float peak = 0.0f;
                    double squares = 0.0;
                    for (std::size_t sample = 0; sample < samples; ++sample)
                    {
                        peak = std::max(peak, std::abs(output[0][sample]));
                        squares += static_cast<double>(output[0][sample]) *
                            output[0][sample];
                    }
                    meter->peak.store(peak, std::memory_order_relaxed);
                    meter->rms.store(samples ? static_cast<float>(std::sqrt(
                        squares / static_cast<double>(samples))) : 0.0f,
                        std::memory_order_relaxed);
                    const bool spectrumEnabled =
                        effects->parameters[effect][0].load(
                            std::memory_order_relaxed) >= 0.5f;
                    if (!spectrumEnabled)
                    {
                        meter->spectrumSize.store(0, std::memory_order_release);
                        continue;
                    }
                    const int windowSelection = std::clamp(static_cast<int>(
                        effects->parameters[effect][1].load(
                            std::memory_order_relaxed)), 0, 2);
                    const std::uint32_t windowFrames = windowSelection == 0
                        ? 512u : windowSelection == 1 ? 1024u : 2048u;
                    EffectNode::LongDelay* storage =
                        effects->longDelays[effect].load(std::memory_order_acquire);
                    if (!storage || storage->samples.size() < 6144)
                        continue;
                    std::vector<float>& scratch = storage->samples;
                    std::uint32_t write = effects->delayPositions[effect] %
                        windowFrames;
                    for (ma_uint32 frame = 0; frame < frames; ++frame)
                    {
                        float mono = 0.0f;
                        for (ma_uint32 channel = 0; channel < channels; ++channel)
                            mono += output[0][static_cast<std::size_t>(frame) *
                                channels + channel];
                        scratch[write] = mono / std::max(channels, 1u);
                        write = (write + 1) % windowFrames;
                    }
                    effects->delayPositions[effect] = write;
                    effects->analysisFilled[effect] = std::min<std::uint32_t>(
                        windowFrames, effects->analysisFilled[effect] + frames);
                    effects->analysisSinceSpectrum[effect] += frames;
                    if (effects->analysisFilled[effect] < windowFrames ||
                        effects->analysisSinceSpectrum[effect] < windowFrames)
                        continue;
                    effects->analysisSinceSpectrum[effect] = 0;
                    float* real = scratch.data() + 2048;
                    float* imaginary = scratch.data() + 4096;
                    for (std::uint32_t index = 0; index < windowFrames; ++index)
                    {
                        const float window = 0.5f - 0.5f * std::cos(
                            6.2831853071795864769f * index /
                            static_cast<float>(windowFrames - 1));
                        real[index] = scratch[(write + index) % windowFrames] *
                            window;
                        imaginary[index] = 0.0f;
                    }
                    for (std::uint32_t index = 1, reverse = 0;
                         index < windowFrames; ++index)
                    {
                        std::uint32_t bit = windowFrames >> 1;
                        while (reverse & bit) { reverse ^= bit; bit >>= 1; }
                        reverse ^= bit;
                        if (index < reverse)
                        {
                            std::swap(real[index], real[reverse]);
                            std::swap(imaginary[index], imaginary[reverse]);
                        }
                    }
                    for (std::uint32_t length = 2; length <= windowFrames;
                         length <<= 1)
                    {
                        const float angle = -6.2831853071795864769f / length;
                        const float stepReal = std::cos(angle);
                        const float stepImaginary = std::sin(angle);
                        for (std::uint32_t begin = 0; begin < windowFrames;
                             begin += length)
                        {
                            float twiddleReal = 1.0f;
                            float twiddleImaginary = 0.0f;
                            for (std::uint32_t offset = 0;
                                 offset < length / 2; ++offset)
                            {
                                const std::uint32_t even = begin + offset;
                                const std::uint32_t odd = even + length / 2;
                                const float oddReal = real[odd] * twiddleReal -
                                    imaginary[odd] * twiddleImaginary;
                                const float oddImaginary = real[odd] *
                                    twiddleImaginary + imaginary[odd] * twiddleReal;
                                real[odd] = real[even] - oddReal;
                                imaginary[odd] = imaginary[even] - oddImaginary;
                                real[even] += oddReal;
                                imaginary[even] += oddImaginary;
                                const float nextReal = twiddleReal * stepReal -
                                    twiddleImaginary * stepImaginary;
                                twiddleImaginary = twiddleReal * stepImaginary +
                                    twiddleImaginary * stepReal;
                                twiddleReal = nextReal;
                            }
                        }
                    }
                    const std::uint32_t bins = windowFrames / 2 + 1;
                    for (std::uint32_t bin = 0; bin < bins; ++bin)
                    {
                        const float scale = bin == 0 || bin == windowFrames / 2
                            ? 1.0f / windowFrames
                            : 1.41421356237f / windowFrames;
                        meter->spectrum[bin].store(std::hypot(real[bin],
                            imaginary[bin]) * scale, std::memory_order_relaxed);
                    }
                    meter->spectrumSize.store(bins, std::memory_order_release);
                }
            }
        }
        *inputFrames = frames;
        *outputFrames = frames;
    }

    static const ma_node_vtable& effectVtable()
    {
        static const ma_node_vtable vtable = {
            &processEffectsNode, nullptr, 1, 1, 0};
        return vtable;
    }

    struct Clip
    {
        std::uint32_t generation = 1;
        PcmClip pcm;
        std::filesystem::path streamPath;
        std::uint64_t frameCount = 0;
        std::uint32_t sampleRate = 0;
        std::uint32_t channels = 0;
        bool streaming = false;
        bool queued = false;
        ma_pcm_rb queue{};
    };

    struct Voice
    {
        std::uint32_t generation = 1;
        std::uint32_t clipIndex = UINT32_MAX;
        std::uint32_t busIndex = UINT32_MAX;
        std::int32_t priority = 0;
        std::uint64_t serial = 0;
        bool paused = false;
        bool ownsBuffer = false;
        bool rangePending = false;
        bool spatial = false;
        std::uint32_t listenerIndex = 0;
        float baseVolume = 1.0f;
        Vector3 position;
        Vector3 direction{0.0f, 0.0f, -1.0f};
        std::vector<AttenuationPoint> attenuationCurve;
        std::vector<AttenuationPoint> angleAttenuationCurve;
        std::uint64_t rangeBegin = 0;
        std::uint64_t rangeEnd = UINT64_MAX;
        std::uint64_t loopBegin = 0;
        std::uint64_t loopEnd = UINT64_MAX;
        ma_audio_buffer_ref buffer{};
        ma_sound sound{};
        EffectNode effects;
        bool effectsInitialized = false;
    };

    struct Bus
    {
        std::uint32_t generation = 1;
        float volume = 1.0f;
        ma_sound_group group{};
    };

    EngineConfig config;
    ma_engine engine{};
    std::vector<std::unique_ptr<Clip>> clips;
    std::vector<std::unique_ptr<Voice>> voices;
    std::vector<std::unique_ptr<Bus>> buses;
    std::vector<std::uint32_t> clipGenerations;
    std::vector<std::uint32_t> voiceGenerations;
    std::vector<std::uint32_t> busGenerations;
    std::uint64_t nextVoiceSerial = 1;
    std::array<ListenerState, 2> listeners;
    float masterVolume = 1.0f;
    bool muted = false;
    ma_device outputDevice{};
    bool outputDeviceInitialized = false;
    bool outputDeviceRunning = false;
    bool restartDeviceAfterResume = false;
    std::atomic<bool> suspended = false;
    std::atomic<bool> outputDeviceInterrupted = false;
    std::atomic<std::uint64_t> outputDeviceEventSerial = 0;
    std::atomic<bool> reverbEnabled = false;
    std::atomic<float> reverbMix = 0.0f;
    std::atomic<float> reverbDecay = 0.5f;
    std::atomic<float> reverbDamping = 0.25f;
    std::atomic<float> reverbRoomSize = 0.5f;
    std::array<std::vector<float>, 4> reverbDelay;
    std::array<std::uint32_t, 4> reverbPosition{};
    std::array<std::vector<float>, 4> reverbFilter;
    std::atomic<bool> equalizerEnabled = false;
    std::atomic<float> equalizerLowGainDb = 0.0f;
    std::atomic<float> equalizerMidGainDb = 0.0f;
    std::atomic<float> equalizerHighGainDb = 0.0f;
    std::vector<float> equalizerLowState;
    std::vector<float> equalizerHighState;
    std::atomic<bool> compressorEnabled = false;
    std::atomic<float> compressorThresholdDb = -12.0f;
    std::atomic<float> compressorRatio = 4.0f;
    std::atomic<float> compressorAttackSeconds = 0.01f;
    std::atomic<float> compressorReleaseSeconds = 0.1f;
    std::atomic<float> compressorMakeupGainDb = 0.0f;
    std::vector<float> compressorEnvelope;
    std::atomic<bool> echoEnabled = false;
    std::atomic<float> echoDelaySeconds = 0.25f;
    std::atomic<float> echoFeedback = 0.35f;
    std::atomic<float> echoMix = 0.2f;
    std::vector<float> echoDelay;
    std::uint32_t echoPosition = 0;

    void processEffects(float* samples, std::uint64_t frameCount) noexcept
    {
        if (equalizerEnabled.load(std::memory_order_relaxed))
        {
            const float lowGain = std::pow(10.0f,
                std::clamp(equalizerLowGainDb.load(std::memory_order_relaxed), -24.0f, 24.0f) / 20.0f);
            const float midGain = std::pow(10.0f,
                std::clamp(equalizerMidGainDb.load(std::memory_order_relaxed), -24.0f, 24.0f) / 20.0f);
            const float highGain = std::pow(10.0f,
                std::clamp(equalizerHighGainDb.load(std::memory_order_relaxed), -24.0f, 24.0f) / 20.0f);
            const float lowAlpha = 1.0f - std::exp(-2.0f * 3.14159265f * 400.0f / config.sampleRate);
            const float highAlpha = 1.0f - std::exp(-2.0f * 3.14159265f * 4000.0f / config.sampleRate);
            for (std::uint64_t frame = 0; frame < frameCount; ++frame)
                for (std::uint32_t channel = 0; channel < config.channels; ++channel)
                {
                    const std::size_t index = static_cast<std::size_t>(frame) * config.channels + channel;
                    const float input = samples[index];
                    equalizerLowState[channel] += lowAlpha * (input - equalizerLowState[channel]);
                    equalizerHighState[channel] += highAlpha * (input - equalizerHighState[channel]);
                    const float low = equalizerLowState[channel];
                    const float mid = equalizerHighState[channel] - low;
                    const float high = input - equalizerHighState[channel];
                    samples[index] = low * lowGain + mid * midGain + high * highGain;
                }
        }

        if (compressorEnabled.load(std::memory_order_relaxed))
        {
            const float threshold = std::pow(10.0f,
                std::clamp(compressorThresholdDb.load(std::memory_order_relaxed), -80.0f, 0.0f) / 20.0f);
            const float ratio = std::max(compressorRatio.load(std::memory_order_relaxed), 1.0f);
            const float attack = std::max(compressorAttackSeconds.load(std::memory_order_relaxed), 0.0001f);
            const float release = std::max(compressorReleaseSeconds.load(std::memory_order_relaxed), 0.0001f);
            const float attackCoefficient = std::exp(-1.0f / (attack * config.sampleRate));
            const float releaseCoefficient = std::exp(-1.0f / (release * config.sampleRate));
            const float makeup = std::pow(10.0f,
                std::clamp(compressorMakeupGainDb.load(std::memory_order_relaxed), -24.0f, 24.0f) / 20.0f);
            for (std::uint64_t frame = 0; frame < frameCount; ++frame)
                for (std::uint32_t channel = 0; channel < config.channels; ++channel)
                {
                    const std::size_t index = static_cast<std::size_t>(frame) * config.channels + channel;
                    const float level = std::abs(samples[index]);
                    const float coefficient = level > compressorEnvelope[channel]
                        ? attackCoefficient : releaseCoefficient;
                    compressorEnvelope[channel] = coefficient * compressorEnvelope[channel] +
                        (1.0f - coefficient) * level;
                    float gain = 1.0f;
                    if (compressorEnvelope[channel] > threshold)
                    {
                        const float compressed = threshold +
                            (compressorEnvelope[channel] - threshold) / ratio;
                        gain = compressed / compressorEnvelope[channel];
                    }
                    samples[index] *= gain * makeup;
                }
        }

        if (echoEnabled.load(std::memory_order_relaxed))
        {
            const std::uint32_t delayFrames = std::clamp<std::uint32_t>(
                static_cast<std::uint32_t>(std::max(0.001f,
                    echoDelaySeconds.load(std::memory_order_relaxed)) * config.sampleRate),
                1, config.sampleRate * 2);
            const float feedback = std::clamp(echoFeedback.load(std::memory_order_relaxed), 0.0f, 0.95f);
            const float mix = std::clamp(echoMix.load(std::memory_order_relaxed), 0.0f, 1.0f);
            for (std::uint64_t frame = 0; frame < frameCount; ++frame)
            {
                const std::uint32_t position = echoPosition % delayFrames;
                for (std::uint32_t channel = 0; channel < config.channels; ++channel)
                {
                    const std::size_t index = static_cast<std::size_t>(frame) * config.channels + channel;
                    const std::size_t delayIndex = static_cast<std::size_t>(position) * config.channels + channel;
                    const float dry = samples[index];
                    const float delayed = echoDelay[delayIndex];
                    echoDelay[delayIndex] = dry + delayed * feedback;
                    samples[index] = dry * (1.0f - mix) + delayed * mix;
                }
                echoPosition = (echoPosition + 1) % delayFrames;
            }
        }

        if (!reverbEnabled.load(std::memory_order_relaxed))
            return;
        const float mix = std::clamp(reverbMix.load(std::memory_order_relaxed), 0.0f, 1.0f);
        const float decay = std::clamp(reverbDecay.load(std::memory_order_relaxed), 0.0f, 0.98f);
        const float damping = std::clamp(reverbDamping.load(std::memory_order_relaxed), 0.0f, 0.99f);
        const float room = std::clamp(reverbRoomSize.load(std::memory_order_relaxed), 0.0f, 1.0f);
        static constexpr std::array<float, 4> delaySeconds{0.0297f, 0.0371f, 0.0411f, 0.0437f};
        for (std::uint64_t frame = 0; frame < frameCount; ++frame)
        {
            for (std::uint32_t channel = 0; channel < config.channels; ++channel)
            {
                const std::size_t sampleIndex = static_cast<std::size_t>(frame) * config.channels + channel;
                const float dry = samples[sampleIndex];
                float wet = 0.0f;
                for (std::size_t line = 0; line < reverbDelay.size(); ++line)
                {
                    const std::uint32_t delayFrames = std::max<std::uint32_t>(1,
                        static_cast<std::uint32_t>(config.sampleRate * delaySeconds[line] * (0.5f + room)));
                    const std::uint32_t position = reverbPosition[line] % delayFrames;
                    const std::size_t delayIndex = static_cast<std::size_t>(position) * config.channels + channel;
                    const float delayed = reverbDelay[line][delayIndex];
                    const float filtered = delayed * (1.0f - damping) + reverbFilter[line][channel] * damping;
                    reverbFilter[line][channel] = filtered;
                    reverbDelay[line][delayIndex] = dry + filtered * decay;
                    wet += filtered;
                }
                samples[sampleIndex] = dry * (1.0f - mix) + wet * (mix * 0.25f);
            }
            for (std::size_t line = 0; line < reverbDelay.size(); ++line)
            {
                const std::uint32_t delayFrames = std::max<std::uint32_t>(1,
                    static_cast<std::uint32_t>(config.sampleRate * delaySeconds[line] * (0.5f + room)));
                reverbPosition[line] = (reverbPosition[line] + 1) % delayFrames;
            }
        }
    }

    static void outputCallback(ma_device* device, void* output, const void*, ma_uint32 frameCount)
    {
        Impl* self = static_cast<Impl*>(device->pUserData);
        if (self->suspended.load(std::memory_order_acquire))
        {
            std::memset(output, 0,
                static_cast<std::size_t>(frameCount) * self->config.channels * sizeof(float));
            return;
        }
        ma_uint64 framesRead = 0;
        const ma_result result = ma_engine_read_pcm_frames(
            &self->engine, output, frameCount, &framesRead);
        if (result != MA_SUCCESS || framesRead < frameCount)
        {
            const std::size_t offset = static_cast<std::size_t>(framesRead) * self->config.channels;
            const std::size_t count = static_cast<std::size_t>(frameCount - framesRead) * self->config.channels;
            std::memset(static_cast<float*>(output) + offset, 0, count * sizeof(float));
        }
        self->processEffects(static_cast<float*>(output), frameCount);
    }

    static void outputNotification(const ma_device_notification* notification)
    {
        Impl* self = static_cast<Impl*>(notification->pDevice->pUserData);
        switch (notification->type)
        {
        case ma_device_notification_type_rerouted:
            self->outputDeviceEventSerial.fetch_add(1, std::memory_order_relaxed);
            break;
        case ma_device_notification_type_interruption_began:
            self->outputDeviceInterrupted.store(true, std::memory_order_release);
            self->outputDeviceEventSerial.fetch_add(1, std::memory_order_relaxed);
            break;
        case ma_device_notification_type_interruption_ended:
            self->outputDeviceInterrupted.store(false, std::memory_order_release);
            self->outputDeviceEventSerial.fetch_add(1, std::memory_order_relaxed);
            break;
        default:
            break;
        }
    }

    explicit Impl(const EngineConfig& requested)
        : config(requested)
    {
        if (config.sampleRate == 0 || config.channels == 0 ||
            config.maxClipFrames == 0 || config.maxEncodedBytes == 0 || config.maxVoices == 0)
            throw std::invalid_argument("audio format and resource limits must be nonzero");

        ma_engine_config engineConfig = ma_engine_config_init();
        engineConfig.noDevice = MA_TRUE;
        engineConfig.sampleRate = config.sampleRate;
        engineConfig.channels = config.channels;
        engineConfig.listenerCount = 2;
        requireSuccess(ma_engine_init(&engineConfig, &engine), "miniaudio engine initialization failed");
        const std::uint32_t maxDelayFrames = static_cast<std::uint32_t>(config.sampleRate * 0.07f);
        for (std::size_t line = 0; line < reverbDelay.size(); ++line)
        {
            reverbDelay[line].resize(static_cast<std::size_t>(maxDelayFrames) * config.channels);
            reverbFilter[line].resize(config.channels);
        }
        equalizerLowState.resize(config.channels);
        equalizerHighState.resize(config.channels);
        compressorEnvelope.resize(config.channels);
        echoDelay.resize(static_cast<std::size_t>(config.sampleRate) * 2 * config.channels);
    }

    ~Impl()
    {
        if (outputDeviceRunning)
            ma_device_stop(&outputDevice);
        if (outputDeviceInitialized)
            ma_device_uninit(&outputDevice);
        for (const std::unique_ptr<Voice>& voice : voices)
            if (voice)
            {
                ma_sound_uninit(&voice->sound);
                if (voice->effectsInitialized)
                    ma_node_uninit(&voice->effects.base, nullptr);
                if (voice->ownsBuffer)
                    ma_audio_buffer_ref_uninit(&voice->buffer);
            }
        for (const std::unique_ptr<Bus>& bus : buses)
            if (bus)
                ma_sound_group_uninit(&bus->group);
        for (const std::unique_ptr<Clip>& clip : clips)
            if (clip && clip->queued)
                ma_pcm_rb_uninit(&clip->queue);
        ma_engine_uninit(&engine);
    }

    Clip* find(ClipHandle handle) const noexcept
    {
        if (handle.index >= clips.size() || !clips[handle.index])
            return nullptr;
        Clip* clip = clips[handle.index].get();
        return clip->generation == handle.generation ? clip : nullptr;
    }

    Voice* find(VoiceHandle handle) const noexcept
    {
        if (handle.index >= voices.size() || !voices[handle.index])
            return nullptr;
        Voice* voice = voices[handle.index].get();
        return voice->generation == handle.generation ? voice : nullptr;
    }

    Bus* find(BusHandle handle) const noexcept
    {
        if (handle.index >= buses.size() || !buses[handle.index])
            return nullptr;
        Bus* bus = buses[handle.index].get();
        return bus->generation == handle.generation ? bus : nullptr;
    }

    void applyPendingRange(Voice* voice) noexcept
    {
        if (!voice || !voice->rangePending)
            return;
        ma_data_source* source = ma_sound_get_data_source(&voice->sound);
        if (!source)
            return;
        const ma_result rangeResult = ma_data_source_set_range_in_pcm_frames(
            source, voice->rangeBegin, voice->rangeEnd);
        const ma_result loopResult = ma_data_source_set_loop_point_in_pcm_frames(
            source, voice->loopBegin, voice->loopEnd);
        if (rangeResult == MA_SUCCESS && loopResult == MA_SUCCESS)
            voice->rangePending = false;
    }

    static float evaluateAttenuationCurve(
        std::span<const AttenuationPoint> curve, float distance) noexcept
    {
        if (curve.empty())
            return 1.0f;
        if (distance <= curve.front().distance)
            return curve.front().gain;
        for (std::size_t index = 1; index < curve.size(); ++index)
        {
            if (distance <= curve[index].distance)
            {
                const AttenuationPoint& left = curve[index - 1];
                const AttenuationPoint& right = curve[index];
                const float alpha = (distance - left.distance) /
                    (right.distance - left.distance);
                return left.gain + (right.gain - left.gain) * alpha;
            }
        }
        return curve.back().gain;
    }

    void applyVoiceVolume(Voice* voice) noexcept
    {
        if (!voice)
            return;
        const ListenerState& listener =
            listeners[std::min<std::uint32_t>(voice->listenerIndex, 1)];
        ma_sound_set_volume(&voice->sound, voice->baseVolume);
        if (voice->spatial && voice->attenuationCurve.empty() &&
            voice->angleAttenuationCurve.empty() &&
            listener.distanceAttenuationCurve.empty() &&
            listener.angleAttenuationCurve.empty())
        {
            ma_sound_set_min_gain(&voice->sound, 0.0f);
            ma_sound_set_max_gain(&voice->sound, 1.0f);
        }
        else if (voice->spatial)
        {
            const float dx = voice->position.x - listener.position.x;
            const float dy = voice->position.y - listener.position.y;
            const float dz = voice->position.z - listener.position.z;
            float gain = evaluateAttenuationCurve(voice->attenuationCurve,
                std::sqrt(dx * dx + dy * dy + dz * dz));
            gain *= evaluateAttenuationCurve(listener.distanceAttenuationCurve,
                std::sqrt(dx * dx + dy * dy + dz * dz));
            if (!voice->angleAttenuationCurve.empty())
            {
                const float listenerDx = -dx;
                const float listenerDy = -dy;
                const float listenerDz = -dz;
                const float listenerLength = std::sqrt(listenerDx * listenerDx +
                    listenerDy * listenerDy + listenerDz * listenerDz);
                const float directionLength = std::sqrt(
                    voice->direction.x * voice->direction.x +
                    voice->direction.y * voice->direction.y +
                    voice->direction.z * voice->direction.z);
                float angleDegrees = 0.0f;
                if (listenerLength > 0.000001f && directionLength > 0.000001f)
                {
                    const float dot = (listenerDx * voice->direction.x +
                        listenerDy * voice->direction.y +
                        listenerDz * voice->direction.z) /
                        (listenerLength * directionLength);
                    angleDegrees = std::acos(std::clamp(dot, -1.0f, 1.0f)) *
                        (180.0f / 3.14159265358979323846f);
                }
                gain *= evaluateAttenuationCurve(
                    voice->angleAttenuationCurve, angleDegrees);
            }
            if (!listener.angleAttenuationCurve.empty())
            {
                const float emitterLength = std::sqrt(
                    dx * dx + dy * dy + dz * dz);
                const float directionLength = std::sqrt(
                    listener.direction.x * listener.direction.x +
                    listener.direction.y * listener.direction.y +
                    listener.direction.z * listener.direction.z);
                float angleDegrees = 0.0f;
                if (emitterLength > 0.000001f && directionLength > 0.000001f)
                {
                    const float dot = (dx * listener.direction.x +
                        dy * listener.direction.y + dz * listener.direction.z) /
                        (emitterLength * directionLength);
                    angleDegrees = std::acos(std::clamp(dot, -1.0f, 1.0f)) *
                        (180.0f / 3.14159265358979323846f);
                }
                gain *= evaluateAttenuationCurve(
                    listener.angleAttenuationCurve, angleDegrees);
            }
            ma_sound_set_min_gain(&voice->sound, gain);
            ma_sound_set_max_gain(&voice->sound, gain);
        }
    }
};

Engine::Engine(const EngineConfig& config)
    : impl(std::make_unique<Impl>(config))
{
}

Engine::~Engine() = default;

ClipHandle Engine::createClip(PcmClip pcm)
{
    if (pcm.sampleRate == 0 || pcm.channels == 0 || pcm.samples.empty() ||
        pcm.samples.size() % pcm.channels != 0 ||
        pcm.samples.size() / pcm.channels > impl->config.maxClipFrames)
        throw std::invalid_argument("invalid PCM clip");

    auto clip = std::make_unique<Impl::Clip>();
    clip->pcm = std::move(pcm);
    clip->frameCount = clip->pcm.samples.size() / clip->pcm.channels;
    clip->sampleRate = clip->pcm.sampleRate;
    clip->channels = clip->pcm.channels;
    for (std::uint32_t index = 0; index < impl->clips.size(); ++index)
    {
        if (!impl->clips[index])
        {
            clip->generation = impl->clipGenerations[index];
            impl->clips[index] = std::move(clip);
            return {index, impl->clips[index]->generation};
        }
    }

    impl->clips.push_back(std::move(clip));
    impl->clipGenerations.push_back(impl->clips.back()->generation);
    return {static_cast<std::uint32_t>(impl->clips.size() - 1), impl->clips.back()->generation};
}

ClipHandle Engine::createEncodedClip(std::span<const std::byte> encodedData)
{
    if (encodedData.empty() || encodedData.size() > impl->config.maxEncodedBytes)
        throw std::invalid_argument("encoded audio data violates configured limits");

    ma_decoder decoder{};
    const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
    const ma_result initialization = ma_decoder_init_memory(
        encodedData.data(), encodedData.size(), &decoderConfig, &decoder);
    if (initialization != MA_SUCCESS)
        return createClip(decodeFfmpegAudio(encodedData, impl->config.maxClipFrames));

    ma_uint64 frameCount = 0;
    const ma_result lengthResult = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    if (lengthResult != MA_SUCCESS || frameCount == 0 ||
        frameCount > impl->config.maxClipFrames || decoder.outputChannels == 0 ||
        frameCount > std::numeric_limits<std::size_t>::max() / decoder.outputChannels)
    {
        ma_decoder_uninit(&decoder);
        throw std::invalid_argument("encoded audio has invalid or unbounded dimensions");
    }

    PcmClip pcm;
    pcm.sampleRate = decoder.outputSampleRate;
    pcm.channels = decoder.outputChannels;
    pcm.samples.resize(static_cast<std::size_t>(frameCount) * pcm.channels);
    ma_uint64 framesRead = 0;
    const ma_result readResult = ma_decoder_read_pcm_frames(
        &decoder, pcm.samples.data(), frameCount, &framesRead);
    ma_decoder_uninit(&decoder);
    if (readResult != MA_SUCCESS || framesRead != frameCount)
        throw std::invalid_argument("encoded audio could not be decoded completely");
    return createClip(std::move(pcm));
}

ClipHandle Engine::createStreamingClip(const std::filesystem::path& path)
{
    if (!std::filesystem::is_regular_file(path) ||
        std::filesystem::file_size(path) == 0 ||
        std::filesystem::file_size(path) > impl->config.maxEncodedBytes)
        throw std::invalid_argument("streaming audio file violates configured limits");

    ma_decoder decoder{};
#if defined(_WIN32)
    const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
    requireSuccess(ma_decoder_init_file_w(path.c_str(), &decoderConfig, &decoder),
        "miniaudio streaming metadata initialization failed");
#else
    const ma_decoder_config decoderConfig = ma_decoder_config_init(ma_format_f32, 0, 0);
    requireSuccess(ma_decoder_init_file(path.c_str(), &decoderConfig, &decoder),
        "miniaudio streaming metadata initialization failed");
#endif
    ma_uint64 frameCount = 0;
    const ma_result lengthResult = ma_decoder_get_length_in_pcm_frames(&decoder, &frameCount);
    const std::uint32_t sampleRate = decoder.outputSampleRate;
    const std::uint32_t channels = decoder.outputChannels;
    ma_decoder_uninit(&decoder);
    if (lengthResult != MA_SUCCESS || frameCount == 0 ||
        frameCount > impl->config.maxClipFrames || sampleRate == 0 || channels == 0)
        throw std::invalid_argument("streaming audio has invalid or unbounded dimensions");

    auto clip = std::make_unique<Impl::Clip>();
    clip->streamPath = path;
    clip->frameCount = frameCount;
    clip->sampleRate = sampleRate;
    clip->channels = channels;
    clip->streaming = true;
    for (std::uint32_t index = 0; index < impl->clips.size(); ++index)
    {
        if (!impl->clips[index])
        {
            clip->generation = impl->clipGenerations[index];
            impl->clips[index] = std::move(clip);
            return {index, impl->clips[index]->generation};
        }
    }
    impl->clips.push_back(std::move(clip));
    impl->clipGenerations.push_back(impl->clips.back()->generation);
    return {static_cast<std::uint32_t>(impl->clips.size() - 1), impl->clips.back()->generation};
}

ClipHandle Engine::createQueuedClip(std::uint32_t sampleRate, std::uint32_t channels,
    std::uint64_t lengthFrames, std::uint32_t capacityFrames)
{
    if (sampleRate == 0 || channels == 0 || channels > 8 ||
        lengthFrames == 0 || capacityFrames == 0)
        throw std::invalid_argument("queued audio format and dimensions must be nonzero");

    auto clip = std::make_unique<Impl::Clip>();
    clip->frameCount = lengthFrames;
    clip->sampleRate = sampleRate;
    clip->channels = channels;
    requireSuccess(ma_pcm_rb_init(ma_format_f32, channels, capacityFrames,
        nullptr, nullptr, &clip->queue), "queued audio ring initialization failed");
    ma_pcm_rb_set_sample_rate(&clip->queue, sampleRate);
    clip->queued = true;
    for (std::uint32_t index = 0; index < impl->clips.size(); ++index)
    {
        if (!impl->clips[index])
        {
            clip->generation = impl->clipGenerations[index];
            impl->clips[index] = std::move(clip);
            return {index, impl->clips[index]->generation};
        }
    }
    impl->clips.push_back(std::move(clip));
    impl->clipGenerations.push_back(impl->clips.back()->generation);
    return {static_cast<std::uint32_t>(impl->clips.size() - 1), impl->clips.back()->generation};
}

std::uint32_t Engine::submitQueuedFrames(ClipHandle handle, std::span<const float> samples)
{
    Impl::Clip* clip = impl->find(handle);
    if (!clip || !clip->queued || samples.empty() || samples.size() % clip->channels != 0)
        return 0;
    std::uint64_t remaining = samples.size() / clip->channels;
    std::uint64_t submitted = 0;
    while (remaining != 0)
    {
        ma_uint32 count = static_cast<ma_uint32>(std::min<std::uint64_t>(
            remaining, std::numeric_limits<ma_uint32>::max()));
        void* destination = nullptr;
        if (ma_pcm_rb_acquire_write(&clip->queue, &count, &destination) != MA_SUCCESS || count == 0)
            break;
        std::memcpy(destination, samples.data() + submitted * clip->channels,
            static_cast<std::size_t>(count) * clip->channels * sizeof(float));
        if (ma_pcm_rb_commit_write(&clip->queue, count) != MA_SUCCESS)
            break;
        submitted += count;
        remaining -= count;
    }
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(
        submitted, std::numeric_limits<std::uint32_t>::max()));
}

std::uint32_t Engine::queuedFrameCapacity(ClipHandle handle) const
{
    const Impl::Clip* clip = impl->find(handle);
    return clip && clip->queued ? ma_pcm_rb_get_subbuffer_size(
        const_cast<ma_pcm_rb*>(&clip->queue)) : 0;
}

std::uint32_t Engine::queuedFramesAvailable(ClipHandle handle) const
{
    const Impl::Clip* clip = impl->find(handle);
    return clip && clip->queued ? ma_pcm_rb_available_read(
        const_cast<ma_pcm_rb*>(&clip->queue)) : 0;
}

bool Engine::resetQueuedClip(ClipHandle handle)
{
    Impl::Clip* clip = impl->find(handle);
    if (!clip || !clip->queued)
        return false;
    ma_pcm_rb_reset(&clip->queue);
    return true;
}

bool Engine::destroyClip(ClipHandle handle)
{
    Impl::Clip* clip = impl->find(handle);
    if (!clip)
        return false;
    for (const std::unique_ptr<Impl::Voice>& voice : impl->voices)
        if (voice && voice->clipIndex == handle.index)
            return false;
    if (clip->queued)
        ma_pcm_rb_uninit(&clip->queue);
    impl->clips[handle.index].reset();
    if (++impl->clipGenerations[handle.index] == 0)
        ++impl->clipGenerations[handle.index];
    return true;
}

BusHandle Engine::createBus(float volume)
{
    auto bus = std::make_unique<Impl::Bus>();
    bus->volume = std::max(volume, 0.0f);
    requireSuccess(ma_sound_group_init(&impl->engine, 0, nullptr, &bus->group),
        "miniaudio bus initialization failed");
    ma_sound_group_set_volume(&bus->group, bus->volume);
    for (std::uint32_t index = 0; index < impl->buses.size(); ++index)
    {
        if (!impl->buses[index])
        {
            bus->generation = impl->busGenerations[index];
            impl->buses[index] = std::move(bus);
            return {index, impl->buses[index]->generation};
        }
    }
    impl->buses.push_back(std::move(bus));
    impl->busGenerations.push_back(impl->buses.back()->generation);
    return {static_cast<std::uint32_t>(impl->buses.size() - 1), impl->buses.back()->generation};
}

bool Engine::destroyBus(BusHandle handle)
{
    Impl::Bus* bus = impl->find(handle);
    if (!bus)
        return false;
    for (const std::unique_ptr<Impl::Voice>& voice : impl->voices)
        if (voice && voice->busIndex == handle.index)
            return false;
    ma_sound_group_uninit(&bus->group);
    impl->buses[handle.index].reset();
    if (++impl->busGenerations[handle.index] == 0)
        ++impl->busGenerations[handle.index];
    return true;
}

bool Engine::setBusVolume(BusHandle handle, float volume)
{
    Impl::Bus* bus = impl->find(handle);
    if (!bus)
        return false;
    bus->volume = std::max(volume, 0.0f);
    ma_sound_group_set_volume(&bus->group, bus->volume);
    return true;
}

float Engine::busVolume(BusHandle handle) const
{
    const Impl::Bus* bus = impl->find(handle);
    return bus ? bus->volume : -1.0f;
}

void Engine::setReverb(const ReverbParameters& parameters) noexcept
{
    impl->reverbMix.store(std::clamp(parameters.mix, 0.0f, 1.0f), std::memory_order_relaxed);
    impl->reverbDecay.store(std::clamp(parameters.decay, 0.0f, 0.98f), std::memory_order_relaxed);
    impl->reverbDamping.store(std::clamp(parameters.damping, 0.0f, 0.99f), std::memory_order_relaxed);
    impl->reverbRoomSize.store(std::clamp(parameters.roomSize, 0.0f, 1.0f), std::memory_order_relaxed);
    impl->reverbEnabled.store(parameters.enabled, std::memory_order_release);
}

ReverbParameters Engine::reverb() const noexcept
{
    return {
        .enabled = impl->reverbEnabled.load(std::memory_order_acquire),
        .mix = impl->reverbMix.load(std::memory_order_relaxed),
        .decay = impl->reverbDecay.load(std::memory_order_relaxed),
        .damping = impl->reverbDamping.load(std::memory_order_relaxed),
        .roomSize = impl->reverbRoomSize.load(std::memory_order_relaxed),
    };
}

void Engine::setEqualizer(const EqualizerParameters& parameters) noexcept
{
    impl->equalizerLowGainDb.store(std::clamp(parameters.lowGainDb, -24.0f, 24.0f), std::memory_order_relaxed);
    impl->equalizerMidGainDb.store(std::clamp(parameters.midGainDb, -24.0f, 24.0f), std::memory_order_relaxed);
    impl->equalizerHighGainDb.store(std::clamp(parameters.highGainDb, -24.0f, 24.0f), std::memory_order_relaxed);
    impl->equalizerEnabled.store(parameters.enabled, std::memory_order_release);
}

EqualizerParameters Engine::equalizer() const noexcept
{
    return {
        .enabled = impl->equalizerEnabled.load(std::memory_order_acquire),
        .lowGainDb = impl->equalizerLowGainDb.load(std::memory_order_relaxed),
        .midGainDb = impl->equalizerMidGainDb.load(std::memory_order_relaxed),
        .highGainDb = impl->equalizerHighGainDb.load(std::memory_order_relaxed),
    };
}

void Engine::setCompressor(const CompressorParameters& parameters) noexcept
{
    impl->compressorThresholdDb.store(std::clamp(parameters.thresholdDb, -80.0f, 0.0f), std::memory_order_relaxed);
    impl->compressorRatio.store(std::clamp(parameters.ratio, 1.0f, 50.0f), std::memory_order_relaxed);
    impl->compressorAttackSeconds.store(std::clamp(parameters.attackSeconds, 0.0001f, 2.0f), std::memory_order_relaxed);
    impl->compressorReleaseSeconds.store(std::clamp(parameters.releaseSeconds, 0.0001f, 5.0f), std::memory_order_relaxed);
    impl->compressorMakeupGainDb.store(std::clamp(parameters.makeupGainDb, -24.0f, 24.0f), std::memory_order_relaxed);
    impl->compressorEnabled.store(parameters.enabled, std::memory_order_release);
}

CompressorParameters Engine::compressor() const noexcept
{
    return {
        .enabled = impl->compressorEnabled.load(std::memory_order_acquire),
        .thresholdDb = impl->compressorThresholdDb.load(std::memory_order_relaxed),
        .ratio = impl->compressorRatio.load(std::memory_order_relaxed),
        .attackSeconds = impl->compressorAttackSeconds.load(std::memory_order_relaxed),
        .releaseSeconds = impl->compressorReleaseSeconds.load(std::memory_order_relaxed),
        .makeupGainDb = impl->compressorMakeupGainDb.load(std::memory_order_relaxed),
    };
}

void Engine::setEcho(const EchoParameters& parameters) noexcept
{
    impl->echoDelaySeconds.store(std::clamp(parameters.delaySeconds, 0.001f, 2.0f), std::memory_order_relaxed);
    impl->echoFeedback.store(std::clamp(parameters.feedback, 0.0f, 0.95f), std::memory_order_relaxed);
    impl->echoMix.store(std::clamp(parameters.mix, 0.0f, 1.0f), std::memory_order_relaxed);
    impl->echoEnabled.store(parameters.enabled, std::memory_order_release);
}

EchoParameters Engine::echo() const noexcept
{
    return {
        .enabled = impl->echoEnabled.load(std::memory_order_acquire),
        .delaySeconds = impl->echoDelaySeconds.load(std::memory_order_relaxed),
        .feedback = impl->echoFeedback.load(std::memory_order_relaxed),
        .mix = impl->echoMix.load(std::memory_order_relaxed),
    };
}

VoiceHandle Engine::play(ClipHandle handle, const VoiceParameters& parameters)
{
    Impl::Clip* clip = impl->find(handle);
    if (!clip)
        return {};
    if (parameters.effectCount > parameters.effects.size() ||
        parameters.distortionCount > parameters.distortionLevels.size())
        return {};
    if (parameters.effectCount)
    {
        for (std::uint32_t index = 0; index < parameters.effectCount; ++index)
        {
            const VoiceEffect& effect = parameters.effects[index];
            if (effect.type != VoiceEffectType::Distortion &&
                effect.type != VoiceEffectType::Tremolo &&
                effect.type != VoiceEffectType::Chorus &&
                effect.type != VoiceEffectType::Flanger &&
                effect.type != VoiceEffectType::Compressor &&
                effect.type != VoiceEffectType::Gate &&
                effect.type != VoiceEffectType::Limiter &&
                effect.type != VoiceEffectType::Equalizer &&
                effect.type != VoiceEffectType::Filter &&
                effect.type != VoiceEffectType::PitchShifter &&
                effect.type != VoiceEffectType::Echo &&
                effect.type != VoiceEffectType::Reverb &&
                effect.type != VoiceEffectType::Analyzer)
                return {};
            const std::size_t parameterCount = effect.type ==
                VoiceEffectType::Distortion ? 1 :
                effect.type == VoiceEffectType::Tremolo ? 6 :
                effect.type == VoiceEffectType::Compressor ? 5 :
                effect.type == VoiceEffectType::Gate ? 4 :
                effect.type == VoiceEffectType::Limiter ? 2 :
                effect.type == VoiceEffectType::Equalizer ? 5 :
                effect.type == VoiceEffectType::Filter ? 4 :
                effect.type == VoiceEffectType::PitchShifter ? 2 :
                effect.type == VoiceEffectType::Echo ? 7 :
                effect.type == VoiceEffectType::Reverb ? 13 :
                effect.type == VoiceEffectType::Analyzer ? 3 : 3;
            for (std::size_t parameter = 0; parameter < parameterCount; ++parameter)
                if (!std::isfinite(effect.parameters[parameter]))
                    return {};
        }
    }
    else
    {
        for (std::uint32_t index = 0; index < parameters.distortionCount; ++index)
            if (!std::isfinite(parameters.distortionLevels[index]))
                return {};
    }

    std::uint32_t liveVoiceCount = 0;
    std::uint32_t stealIndex = UINT32_MAX;
    for (std::uint32_t index = 0; index < impl->voices.size(); ++index)
    {
        const std::unique_ptr<Impl::Voice>& existing = impl->voices[index];
        if (!existing)
            continue;
        ++liveVoiceCount;
        if (stealIndex == UINT32_MAX || existing->priority < impl->voices[stealIndex]->priority ||
            (existing->priority == impl->voices[stealIndex]->priority &&
                existing->serial < impl->voices[stealIndex]->serial))
            stealIndex = index;
    }
    if (liveVoiceCount >= impl->config.maxVoices)
    {
        if (stealIndex == UINT32_MAX || parameters.priority < impl->voices[stealIndex]->priority)
            return {};
        Impl::Voice* stolen = impl->voices[stealIndex].get();
        ma_sound_uninit(&stolen->sound);
        if (stolen->effectsInitialized)
            ma_node_uninit(&stolen->effects.base, nullptr);
        if (stolen->ownsBuffer)
            ma_audio_buffer_ref_uninit(&stolen->buffer);
        impl->voices[stealIndex].reset();
        if (++impl->voiceGenerations[stealIndex] == 0)
            ++impl->voiceGenerations[stealIndex];
    }

    auto voice = std::make_unique<Impl::Voice>();
    voice->clipIndex = handle.index;
    Impl::Bus* bus = nullptr;
    if (parameters.bus)
    {
        bus = impl->find(parameters.bus);
        if (!bus)
            return {};
        voice->busIndex = parameters.bus.index;
    }
    voice->priority = parameters.priority;
    voice->serial = impl->nextVoiceSerial++;
    voice->spatial = parameters.spatial;
    voice->listenerIndex = std::min<std::uint32_t>(parameters.listenerIndex, 1);
    voice->baseVolume = std::max(parameters.volume, 0.0f);
    voice->position = parameters.position;
    voice->direction = parameters.direction;
    const std::uint64_t clipFrames = clip->frameCount;
    const std::uint64_t rangeBegin = std::min(parameters.rangeBeginFrame, clipFrames);
    const std::uint64_t rangeEnd = std::min(parameters.rangeEndFrame, clipFrames);
    const std::uint64_t loopBegin = std::min(parameters.loopBeginFrame, clipFrames);
    const std::uint64_t loopEnd = std::min(parameters.loopEndFrame, clipFrames);
    bool soundInitialized = false;
    try
    {
    if (clip->queued)
    {
        requireSuccess(ma_sound_init_from_data_source(&impl->engine,
            reinterpret_cast<ma_data_source*>(&clip->queue), 0,
            bus ? &bus->group : nullptr, &voice->sound),
            "queued audio voice initialization failed");
        soundInitialized = true;
    }
    else if (clip->streaming)
    {
#if defined(_WIN32)
        requireSuccess(ma_sound_init_from_file_w(&impl->engine, clip->streamPath.c_str(),
            MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC, bus ? &bus->group : nullptr,
            nullptr, &voice->sound), "miniaudio streaming voice initialization failed");
#else
        requireSuccess(ma_sound_init_from_file(&impl->engine, clip->streamPath.c_str(),
            MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_ASYNC, bus ? &bus->group : nullptr,
            nullptr, &voice->sound), "miniaudio streaming voice initialization failed");
#endif
        soundInitialized = true;
        voice->rangeBegin = rangeBegin;
        voice->rangeEnd = rangeEnd;
        voice->loopBegin = loopBegin;
        voice->loopEnd = loopEnd;
        voice->rangePending = rangeBegin < rangeEnd && loopBegin < loopEnd;
    }
    else
    {
        requireSuccess(ma_audio_buffer_ref_init(ma_format_f32, clip->channels,
            clip->pcm.samples.data(), clipFrames, &voice->buffer),
            "miniaudio voice buffer initialization failed");
        // ma_audio_buffer_ref_init() in the pinned miniaudio revision leaves
        // sampleRate at zero (its own 0.12 TODO).  Zero makes the engine treat
        // every decoded buffer as though it were already at the output rate,
        // so Roblox's 22.05 kHz stock sounds played about 2.18x too fast and
        // too high through the 48 kHz mixer.
        voice->buffer.sampleRate = clip->sampleRate;
        voice->ownsBuffer = true;
        if (rangeBegin < rangeEnd)
            requireSuccess(ma_data_source_set_range_in_pcm_frames(
                reinterpret_cast<ma_data_source*>(&voice->buffer), rangeBegin, rangeEnd),
                "miniaudio voice range initialization failed");
        if (loopBegin < loopEnd)
            requireSuccess(ma_data_source_set_loop_point_in_pcm_frames(
                reinterpret_cast<ma_data_source*>(&voice->buffer), loopBegin, loopEnd),
                "miniaudio voice loop range initialization failed");
        requireSuccess(ma_sound_init_from_data_source(&impl->engine,
            reinterpret_cast<ma_data_source*>(&voice->buffer), 0,
            bus ? &bus->group : nullptr, &voice->sound),
            "miniaudio voice initialization failed");
        soundInitialized = true;
    }
    impl->applyVoiceVolume(voice.get());
    ma_sound_set_pitch(&voice->sound, std::max(parameters.pitch, 0.01f));
    ma_sound_set_looping(&voice->sound, parameters.looping ? MA_TRUE : MA_FALSE);
    ma_sound_set_spatialization_enabled(&voice->sound, parameters.spatial ? MA_TRUE : MA_FALSE);
    ma_sound_set_pinned_listener_index(&voice->sound, voice->listenerIndex);
    ma_sound_set_position(&voice->sound, parameters.position.x, parameters.position.y, parameters.position.z);
    ma_sound_set_direction(&voice->sound, parameters.direction.x,
        parameters.direction.y, parameters.direction.z);
    ma_sound_set_velocity(&voice->sound, parameters.velocity.x, parameters.velocity.y, parameters.velocity.z);
    ma_sound_set_min_distance(&voice->sound, std::max(parameters.minDistance, 0.001f));
    ma_sound_set_max_distance(&voice->sound, std::max(parameters.maxDistance, parameters.minDistance));
    ma_sound_set_rolloff(&voice->sound, std::max(parameters.rolloff, 0.0f));
    ma_sound_set_doppler_factor(&voice->sound, std::max(parameters.dopplerFactor, 0.0f));
    ma_sound_set_attenuation_model(&voice->sound,
        parameters.attenuation == AttenuationModel::Linear
            ? ma_attenuation_model_linear
            : ma_attenuation_model_inverse);
    if (std::isfinite(parameters.startMixerTimeSeconds) &&
        parameters.startMixerTimeSeconds >= 0.0)
    {
        const double frames = parameters.startMixerTimeSeconds *
            static_cast<double>(impl->config.sampleRate);
        ma_sound_set_start_time_in_pcm_frames(&voice->sound,
            static_cast<ma_uint64>(std::max(frames, 0.0)));
    }
    {
        const ma_uint32 channels = impl->config.channels;
        ma_node_config nodeConfig = ma_node_config_init();
        nodeConfig.vtable = &Impl::effectVtable();
        nodeConfig.pInputChannels = &channels;
        nodeConfig.pOutputChannels = &channels;
        requireSuccess(ma_node_init(ma_engine_get_node_graph(&impl->engine),
            &nodeConfig, nullptr, &voice->effects.base),
            "voice effect node initialization failed");
        voice->effectsInitialized = true;
        voice->effects.sampleRate = impl->config.sampleRate;
        voice->effects.channels = impl->config.channels;
        const std::size_t modulationDelaySamples = std::max<std::size_t>(
            static_cast<std::size_t>(impl->config.sampleRate) / 20 + 2,
            2050) * impl->config.channels;
        for (std::vector<float>& delay : voice->effects.delays)
            delay.resize(modulationDelaySamples);
        for (std::vector<float>& state : voice->effects.states)
            state.resize(impl->config.channels);
        for (std::vector<float>& state : voice->effects.secondaryStates)
            state.resize(impl->config.channels);
        for (std::vector<float>& state : voice->effects.tertiaryStates)
            state.resize(impl->config.channels);
        const std::uint32_t effectCount = std::min<std::uint32_t>(
            parameters.effectCount ? parameters.effectCount :
                parameters.distortionCount,
            static_cast<std::uint32_t>(voice->effects.types.size()));
        for (std::uint32_t index = 0; index < effectCount; ++index)
        {
            const VoiceEffect effect = parameters.effectCount
                ? parameters.effects[index]
                : VoiceEffect{VoiceEffectType::Distortion,
                    {parameters.distortionLevels[index]}, {}};
            voice->effects.types[index].store(
                static_cast<std::uint8_t>(effect.type),
                std::memory_order_relaxed);
            for (std::size_t parameter = 0;
                 parameter < effect.parameters.size(); ++parameter)
                voice->effects.parameters[index][parameter].store(
                    effect.parameters[parameter], std::memory_order_relaxed);
            if (effect.type == VoiceEffectType::Echo ||
                effect.type == VoiceEffectType::Reverb ||
                effect.type == VoiceEffectType::Analyzer)
            {
                std::unique_ptr<Impl::EffectNode::LongDelay> longDelay(
                    new Impl::EffectNode::LongDelay());
                longDelay->samples.resize(effect.type == VoiceEffectType::Analyzer
                    ? 6144 : (static_cast<std::size_t>(impl->config.sampleRate) *
                        5 + 2) * impl->config.channels);
                longDelay->resetMarker = effect.type == VoiceEffectType::Echo
                    ? static_cast<std::uint32_t>(std::max(
                        effect.parameters[5], 0.0f)) : 0u;
                longDelay->ownerKey = static_cast<std::uint32_t>(
                    std::max(effect.parameters[effect.type ==
                        VoiceEffectType::Echo ? 6 : effect.type ==
                        VoiceEffectType::Reverb ? 12 : 2], 0.0f));
                Impl::EffectNode::LongDelay* published = longDelay.get();
                voice->effects.retainedLongDelays.push_back(std::move(longDelay));
                voice->effects.longDelays[index].store(published,
                    std::memory_order_release);
            }
            if (effect.type == VoiceEffectType::Analyzer && effect.meter)
            {
                voice->effects.retainedMeters.push_back(effect.meter);
                voice->effects.meters[index].store(effect.meter.get(),
                    std::memory_order_release);
            }
        }
        voice->effects.count.store(effectCount, std::memory_order_release);
        ma_node* destination = bus
            ? static_cast<ma_node*>(&bus->group)
            : ma_engine_get_endpoint(&impl->engine);
        requireSuccess(ma_node_attach_output_bus(&voice->effects.base, 0,
            destination, 0), "voice effect output attachment failed");
        requireSuccess(ma_node_attach_output_bus(&voice->sound, 0,
            &voice->effects.base, 0), "voice effect input attachment failed");
    }
    requireSuccess(ma_sound_start(&voice->sound), "miniaudio voice start failed");
    }
    catch (...)
    {
        if (soundInitialized)
            ma_sound_uninit(&voice->sound);
        if (voice->effectsInitialized)
            ma_node_uninit(&voice->effects.base, nullptr);
        if (voice->ownsBuffer)
            ma_audio_buffer_ref_uninit(&voice->buffer);
        throw;
    }

    for (std::uint32_t index = 0; index < impl->voices.size(); ++index)
    {
        if (!impl->voices[index])
        {
            voice->generation = impl->voiceGenerations[index];
            impl->voices[index] = std::move(voice);
            return {index, impl->voices[index]->generation};
        }
    }

    impl->voices.push_back(std::move(voice));
    impl->voiceGenerations.push_back(impl->voices.back()->generation);
    return {static_cast<std::uint32_t>(impl->voices.size() - 1), impl->voices.back()->generation};
}

bool Engine::destroyVoice(VoiceHandle handle)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice)
        return false;
    ma_sound_uninit(&voice->sound);
    if (voice->effectsInitialized)
        ma_node_uninit(&voice->effects.base, nullptr);
    if (voice->ownsBuffer)
        ma_audio_buffer_ref_uninit(&voice->buffer);
    impl->voices[handle.index].reset();
    if (++impl->voiceGenerations[handle.index] == 0)
        ++impl->voiceGenerations[handle.index];
    return true;
}

bool Engine::pause(VoiceHandle handle)
{
    Impl::Voice* voice = impl->find(handle);
    impl->applyPendingRange(voice);
    if (!voice || ma_sound_stop(&voice->sound) != MA_SUCCESS)
        return false;
    voice->paused = true;
    return true;
}

bool Engine::resume(VoiceHandle handle)
{
    Impl::Voice* voice = impl->find(handle);
    impl->applyPendingRange(voice);
    if (!voice || !voice->paused || ma_sound_start(&voice->sound) != MA_SUCCESS)
        return false;
    voice->paused = false;
    return true;
}

bool Engine::stop(VoiceHandle handle)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice || ma_sound_stop(&voice->sound) != MA_SUCCESS)
        return false;
    voice->paused = false;
    return true;
}

bool Engine::seekFrames(VoiceHandle handle, std::uint64_t frame)
{
    Impl::Voice* voice = impl->find(handle);
    impl->applyPendingRange(voice);
    return voice && ma_sound_seek_to_pcm_frame(&voice->sound, frame) == MA_SUCCESS;
}

bool Engine::setVoiceTransform(VoiceHandle handle, Vector3 position, Vector3 velocity)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice)
        return false;
    voice->position = position;
    ma_sound_set_position(&voice->sound, position.x, position.y, position.z);
    ma_sound_set_velocity(&voice->sound, velocity.x, velocity.y, velocity.z);
    impl->applyVoiceVolume(voice);
    return true;
}

bool Engine::setVoiceDirection(VoiceHandle handle, Vector3 direction)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice || !std::isfinite(direction.x) || !std::isfinite(direction.y) ||
        !std::isfinite(direction.z))
        return false;
    voice->direction = direction;
    ma_sound_set_direction(&voice->sound, direction.x, direction.y, direction.z);
    impl->applyVoiceVolume(voice);
    return true;
}

bool Engine::setVoiceSpatialModel(VoiceHandle handle, float minDistance, float maxDistance,
    float rolloff, float dopplerFactor, AttenuationModel attenuation)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice)
        return false;
    voice->attenuationCurve.clear();
    ma_sound_set_min_gain(&voice->sound, 0.0f);
    ma_sound_set_max_gain(&voice->sound, 1.0f);
    minDistance = std::max(minDistance, 0.001f);
    ma_sound_set_min_distance(&voice->sound, minDistance);
    ma_sound_set_max_distance(&voice->sound, std::max(maxDistance, minDistance));
    ma_sound_set_rolloff(&voice->sound, std::max(rolloff, 0.0f));
    ma_sound_set_doppler_factor(&voice->sound, std::max(dopplerFactor, 0.0f));
    ma_sound_set_attenuation_model(&voice->sound,
        attenuation == AttenuationModel::Linear
            ? ma_attenuation_model_linear
            : ma_attenuation_model_inverse);
    impl->applyVoiceVolume(voice);
    return true;
}

bool Engine::setVoiceAttenuationCurve(VoiceHandle handle,
    std::span<const AttenuationPoint> curve)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice || curve.size() > 256)
        return false;
    if (curve.empty())
    {
        voice->attenuationCurve.clear();
        impl->applyVoiceVolume(voice);
        return true;
    }
    float previousDistance = -1.0f;
    for (const AttenuationPoint& point : curve)
    {
        if (!std::isfinite(point.distance) || !std::isfinite(point.gain) ||
            point.distance < 0.0f || point.gain < 0.0f ||
            point.distance <= previousDistance)
            return false;
        previousDistance = point.distance;
    }
    voice->attenuationCurve.assign(curve.begin(), curve.end());
    // Keep miniaudio's spatializer active for panning and doppler, then clamp
    // its distance stage to the exact gain sampled from Roblox's authored
    // piecewise-linear curve.
    ma_sound_set_attenuation_model(&voice->sound, ma_attenuation_model_inverse);
    impl->applyVoiceVolume(voice);
    return true;
}

bool Engine::setVoiceAngleAttenuationCurve(VoiceHandle handle,
    std::span<const AttenuationPoint> curve)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice || curve.size() > 256)
        return false;
    if (curve.empty())
    {
        voice->angleAttenuationCurve.clear();
        impl->applyVoiceVolume(voice);
        return true;
    }
    if (curve.size() < 2)
        return false;
    float previousAngle = -1.0f;
    for (const AttenuationPoint& point : curve)
    {
        if (!std::isfinite(point.distance) || !std::isfinite(point.gain) ||
            point.distance < 0.0f || point.distance > 180.0f ||
            point.gain < 0.0f || point.distance <= previousAngle)
            return false;
        previousAngle = point.distance;
    }
    voice->angleAttenuationCurve.assign(curve.begin(), curve.end());
    ma_sound_set_attenuation_model(&voice->sound, ma_attenuation_model_inverse);
    impl->applyVoiceVolume(voice);
    return true;
}

bool Engine::setVoiceVolume(VoiceHandle handle, float volume)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice)
        return false;
    voice->baseVolume = std::max(volume, 0.0f);
    impl->applyVoiceVolume(voice);
    return true;
}

bool Engine::setVoicePitch(VoiceHandle handle, float pitch)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice)
        return false;
    ma_sound_set_pitch(&voice->sound, std::max(pitch, 0.01f));
    return true;
}

bool Engine::setVoiceDistortion(VoiceHandle handle,
    std::span<const float> levels)
{
    std::array<VoiceEffect, 32> effects{};
    if (levels.size() > effects.size())
        return false;
    for (std::size_t index = 0; index < levels.size(); ++index)
    {
        if (!std::isfinite(levels[index]))
            return false;
        effects[index].type = VoiceEffectType::Distortion;
        effects[index].parameters[0] = std::clamp(levels[index], 0.0f, 1.0f);
    }
    return setVoiceEffects(handle,
        std::span<const VoiceEffect>(effects.data(), levels.size()));
}

bool Engine::setVoiceEffects(VoiceHandle handle,
    std::span<const VoiceEffect> effects)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice || !voice->effectsInitialized ||
        effects.size() > voice->effects.types.size())
        return false;
    for (const VoiceEffect& effect : effects)
    {
        if (effect.type != VoiceEffectType::Distortion &&
            effect.type != VoiceEffectType::Tremolo &&
            effect.type != VoiceEffectType::Chorus &&
            effect.type != VoiceEffectType::Flanger &&
            effect.type != VoiceEffectType::Compressor &&
            effect.type != VoiceEffectType::Gate &&
            effect.type != VoiceEffectType::Limiter &&
            effect.type != VoiceEffectType::Equalizer &&
            effect.type != VoiceEffectType::Filter &&
            effect.type != VoiceEffectType::PitchShifter &&
            effect.type != VoiceEffectType::Echo &&
            effect.type != VoiceEffectType::Reverb &&
            effect.type != VoiceEffectType::Analyzer)
            return false;
        const std::size_t parameterCount = effect.type ==
            VoiceEffectType::Distortion ? 1 :
            effect.type == VoiceEffectType::Tremolo ? 6 :
            effect.type == VoiceEffectType::Compressor ? 5 :
            effect.type == VoiceEffectType::Gate ? 4 :
            effect.type == VoiceEffectType::Limiter ? 2 :
            effect.type == VoiceEffectType::Equalizer ? 5 :
            effect.type == VoiceEffectType::Filter ? 4 :
            effect.type == VoiceEffectType::PitchShifter ? 2 :
            effect.type == VoiceEffectType::Echo ? 7 :
            effect.type == VoiceEffectType::Reverb ? 13 : 3;
        for (std::size_t parameter = 0; parameter < parameterCount; ++parameter)
            if (!std::isfinite(effect.parameters[parameter]))
                return false;
    }
    for (std::size_t index = 0; index < effects.size(); ++index)
    {
        if (effects[index].type == VoiceEffectType::Echo ||
            effects[index].type == VoiceEffectType::Reverb ||
            effects[index].type == VoiceEffectType::Analyzer)
        {
            const bool echo = effects[index].type == VoiceEffectType::Echo;
            const bool reverb = effects[index].type == VoiceEffectType::Reverb;
            const std::uint32_t resetMarker = echo
                ? static_cast<std::uint32_t>(std::max(
                    effects[index].parameters[5], 0.0f)) : 0u;
            const std::uint32_t ownerKey = static_cast<std::uint32_t>(
                std::max(effects[index].parameters[echo ? 6 : reverb ? 12 : 2],
                    0.0f));
            Impl::EffectNode::LongDelay* current =
                voice->effects.longDelays[index].load(std::memory_order_acquire);
            const VoiceEffectType previousType = static_cast<VoiceEffectType>(
                voice->effects.types[index].load(std::memory_order_relaxed));
            if (!current || previousType != effects[index].type ||
                current->resetMarker != resetMarker ||
                current->ownerKey != ownerKey)
            {
                std::unique_ptr<Impl::EffectNode::LongDelay> longDelay(
                    new Impl::EffectNode::LongDelay());
                longDelay->samples.resize(effects[index].type ==
                    VoiceEffectType::Analyzer ? 6144 :
                    (static_cast<std::size_t>(impl->config.sampleRate) * 5 + 2) *
                        impl->config.channels);
                longDelay->resetMarker = resetMarker;
                longDelay->ownerKey = ownerKey;
                Impl::EffectNode::LongDelay* published = longDelay.get();
                voice->effects.retainedLongDelays.push_back(std::move(longDelay));
                voice->effects.longDelays[index].store(published,
                    std::memory_order_release);
            }
        }
        if (effects[index].type == VoiceEffectType::Analyzer &&
            effects[index].meter)
        {
            MeterState* current = voice->effects.meters[index].load(
                std::memory_order_acquire);
            if (current != effects[index].meter.get())
            {
                voice->effects.retainedMeters.push_back(effects[index].meter);
                voice->effects.meters[index].store(effects[index].meter.get(),
                    std::memory_order_release);
            }
        }
        voice->effects.types[index].store(static_cast<std::uint8_t>(
            effects[index].type), std::memory_order_relaxed);
        for (std::size_t parameter = 0;
             parameter < effects[index].parameters.size(); ++parameter)
            voice->effects.parameters[index][parameter].store(
                effects[index].parameters[parameter], std::memory_order_relaxed);
    }
    voice->effects.count.store(static_cast<std::uint32_t>(effects.size()),
        std::memory_order_release);
    return true;
}

bool Engine::setVoiceLooping(VoiceHandle handle, bool looping)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice)
        return false;
    ma_sound_set_looping(&voice->sound, looping ? MA_TRUE : MA_FALSE);
    return true;
}

bool Engine::scheduleVoiceStop(VoiceHandle handle, double mixerTime) 
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice || !std::isfinite(mixerTime) || mixerTime < 0.0)
        return false;
    const double frames = mixerTime * static_cast<double>(impl->config.sampleRate);
    ma_sound_set_stop_time_in_pcm_frames(&voice->sound,
        static_cast<ma_uint64>(std::max(frames, 0.0)));
    return true;
}

bool Engine::cancelVoiceStop(VoiceHandle handle)
{
    Impl::Voice* voice = impl->find(handle);
    if (!voice)
        return false;
    ma_sound_set_stop_time_in_pcm_frames(&voice->sound, UINT64_MAX);
    return true;
}

bool Engine::isPlaying(VoiceHandle handle) const
{
    Impl::Voice* voice = impl->find(handle);
    impl->applyPendingRange(voice);
    return voice && ma_sound_is_playing(&voice->sound) == MA_TRUE;
}

bool Engine::isPaused(VoiceHandle handle) const
{
    Impl::Voice* voice = impl->find(handle);
    impl->applyPendingRange(voice);
    return voice && voice->paused;
}

bool Engine::isFinished(VoiceHandle handle) const
{
    Impl::Voice* voice = impl->find(handle);
    impl->applyPendingRange(voice);
    return voice && ma_sound_at_end(&voice->sound) == MA_TRUE;
}

std::uint64_t Engine::positionFrames(VoiceHandle handle) const
{
    Impl::Voice* voice = impl->find(handle);
    impl->applyPendingRange(voice);
    ma_uint64 cursor = 0;
    return voice && ma_sound_get_cursor_in_pcm_frames(&voice->sound, &cursor) == MA_SUCCESS ? cursor : 0;
}

std::uint64_t Engine::lengthFrames(VoiceHandle handle) const
{
    Impl::Voice* voice = impl->find(handle);
    impl->applyPendingRange(voice);
    ma_uint64 length = 0;
    return voice && ma_sound_get_length_in_pcm_frames(&voice->sound, &length) == MA_SUCCESS ? length : 0;
}

std::uint64_t Engine::clipLengthFrames(ClipHandle handle) const
{
    const Impl::Clip* clip = impl->find(handle);
    return clip ? clip->frameCount : 0;
}

std::uint32_t Engine::clipSampleRate(ClipHandle handle) const
{
    const Impl::Clip* clip = impl->find(handle);
    return clip ? clip->sampleRate : 0;
}

void Engine::setListener(const ListenerState& listener)
{
    impl->listeners[0] = listener;
    ma_engine_listener_set_position(&impl->engine, 0, listener.position.x, listener.position.y, listener.position.z);
    ma_engine_listener_set_direction(&impl->engine, 0, listener.direction.x, listener.direction.y, listener.direction.z);
    ma_engine_listener_set_world_up(&impl->engine, 0, listener.up.x, listener.up.y, listener.up.z);
    ma_engine_listener_set_velocity(&impl->engine, 0, listener.velocity.x, listener.velocity.y, listener.velocity.z);
    for (const std::unique_ptr<Impl::Voice>& voice : impl->voices)
        if (voice)
            impl->applyVoiceVolume(voice.get());
}

void Engine::setGraphListener(const ListenerState& listener)
{
    impl->listeners[1] = listener;
    ma_engine_listener_set_position(&impl->engine, 1, listener.position.x,
        listener.position.y, listener.position.z);
    ma_engine_listener_set_direction(&impl->engine, 1, listener.direction.x,
        listener.direction.y, listener.direction.z);
    ma_engine_listener_set_world_up(&impl->engine, 1, listener.up.x,
        listener.up.y, listener.up.z);
    ma_engine_listener_set_velocity(&impl->engine, 1, listener.velocity.x,
        listener.velocity.y, listener.velocity.z);
    for (const std::unique_ptr<Impl::Voice>& voice : impl->voices)
        if (voice)
            impl->applyVoiceVolume(voice.get());
}

void Engine::setMasterVolume(float volume)
{
    impl->masterVolume = std::max(volume, 0.0f);
    ma_engine_set_volume(&impl->engine, impl->muted ? 0.0f : impl->masterVolume);
}

float Engine::masterVolume() const noexcept { return impl->masterVolume; }

void Engine::setMuted(bool muted)
{
    impl->muted = muted;
    ma_engine_set_volume(&impl->engine, muted ? 0.0f : impl->masterVolume);
}

bool Engine::muted() const noexcept { return impl->muted; }

std::uint32_t Engine::activeVoiceCount() const noexcept
{
    return static_cast<std::uint32_t>(std::count_if(impl->voices.begin(), impl->voices.end(),
        [](const std::unique_ptr<Impl::Voice>& voice) { return voice != nullptr; }));
}

void Engine::startOutputDevice()
{
    if (impl->suspended.load(std::memory_order_acquire))
    {
        impl->restartDeviceAfterResume = true;
        return;
    }
    if (impl->outputDeviceRunning)
        return;
    if (!impl->outputDeviceInitialized)
    {
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = ma_format_f32;
        config.playback.channels = impl->config.channels;
        config.sampleRate = impl->config.sampleRate;
        config.dataCallback = &Impl::outputCallback;
        config.notificationCallback = &Impl::outputNotification;
        config.pUserData = impl.get();
        requireSuccess(ma_device_init(nullptr, &config, &impl->outputDevice),
            "miniaudio output device initialization failed");
        impl->outputDeviceInitialized = true;
    }
    requireSuccess(ma_device_start(&impl->outputDevice),
        "miniaudio output device start failed");
    impl->outputDeviceRunning = true;
}

void Engine::stopOutputDevice() noexcept
{
    impl->restartDeviceAfterResume = false;
    if (!impl->outputDeviceRunning)
        return;
    ma_device_stop(&impl->outputDevice);
    impl->outputDeviceRunning = false;
}

void Engine::restartOutputDevice()
{
    const bool wasSuspended = impl->suspended.load(std::memory_order_acquire);
    if (wasSuspended)
    {
        impl->restartDeviceAfterResume = true;
        return;
    }
    stopOutputDevice();
    if (impl->outputDeviceInitialized)
    {
        ma_device_uninit(&impl->outputDevice);
        impl->outputDeviceInitialized = false;
    }
    startOutputDevice();
}

void Engine::suspend() noexcept
{
    if (impl->suspended.exchange(true, std::memory_order_acq_rel))
        return;
    impl->restartDeviceAfterResume = impl->outputDeviceRunning;
    if (impl->outputDeviceRunning)
    {
        ma_device_stop(&impl->outputDevice);
        impl->outputDeviceRunning = false;
    }
}

void Engine::resume()
{
    if (!impl->suspended.exchange(false, std::memory_order_acq_rel))
        return;
    const bool restart = impl->restartDeviceAfterResume;
    impl->restartDeviceAfterResume = false;
    if (restart)
        startOutputDevice();
}

bool Engine::suspended() const noexcept
{
    return impl->suspended.load(std::memory_order_acquire);
}

bool Engine::outputDeviceStarted() const noexcept { return impl->outputDeviceRunning; }
bool Engine::outputDeviceInterrupted() const noexcept
{
    return impl->outputDeviceInterrupted.load(std::memory_order_acquire);
}
std::uint64_t Engine::outputDeviceEventSerial() const noexcept
{
    return impl->outputDeviceEventSerial.load(std::memory_order_relaxed);
}

double Engine::mixerTimeSeconds() const noexcept
{
    const std::uint32_t rate = impl->config.sampleRate;
    return rate == 0 ? 0.0 :
        static_cast<double>(ma_engine_get_time_in_pcm_frames(&impl->engine)) /
            static_cast<double>(rate);
}

bool Engine::mix(std::span<float> output) noexcept
{
    if (output.empty() || output.size() % impl->config.channels != 0)
        return false;
    std::fill(output.begin(), output.end(), 0.0f);
    if (impl->suspended.load(std::memory_order_acquire))
        return true;
    const ma_uint64 frames = output.size() / impl->config.channels;
    ma_uint64 framesRead = 0;
    const bool complete = ma_engine_read_pcm_frames(&impl->engine, output.data(), frames, &framesRead) == MA_SUCCESS &&
        framesRead == frames;
    impl->processEffects(output.data(), frames);
    return complete;
}

std::uint32_t Engine::sampleRate() const noexcept { return impl->config.sampleRate; }
std::uint32_t Engine::channels() const noexcept { return impl->config.channels; }

} // namespace RBX::Audio
