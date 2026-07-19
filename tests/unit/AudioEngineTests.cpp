#include "audio/AudioEngine.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cmath>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool near(float value, float expected, float tolerance = 0.0001f)
{
    return std::abs(value - expected) <= tolerance;
}

float channelEnergy(const std::vector<float>& samples, std::size_t channel)
{
    float result = 0.0f;
    for (std::size_t index = channel; index < samples.size(); index += 2)
        result += std::abs(samples[index]);
    return result;
}

std::vector<std::byte> makeMonoPcm16Wav()
{
    const std::array<std::int16_t, 4> pcm{8192, -16384, 24576, -32767};
    std::vector<std::byte> result;
    const auto append = [&result](std::uint32_t value, unsigned bytes) {
        for (unsigned index = 0; index < bytes; ++index)
            result.push_back(static_cast<std::byte>((value >> (index * 8)) & 0xff));
    };
    const auto text = [&result](const char* value) {
        for (unsigned index = 0; index < 4; ++index)
            result.push_back(static_cast<std::byte>(value[index]));
    };
    text("RIFF");
    append(36 + pcm.size() * sizeof(std::int16_t), 4);
    text("WAVE");
    text("fmt ");
    append(16, 4);
    append(1, 2);
    append(1, 2);
    append(48000, 4);
    append(48000 * sizeof(std::int16_t), 4);
    append(sizeof(std::int16_t), 2);
    append(16, 2);
    text("data");
    append(pcm.size() * sizeof(std::int16_t), 4);
    for (const std::int16_t sample : pcm)
        append(static_cast<std::uint16_t>(sample), 2);
    return result;
}

} // namespace

int main()
{
    using namespace RBX::Audio;

    Engine engine({.sampleRate = 48000, .channels = 2});
    require(engine.sampleRate() == 48000 && engine.channels() == 2,
        "engine format must match its explicit configuration");

    ClipHandle clip = engine.createClip({
        .sampleRate = 48000,
        .channels = 1,
        .samples = {0.25f, -0.5f, 0.75f, -1.0f},
    });
    require(static_cast<bool>(clip), "valid PCM must create a clip");

    VoiceHandle voice = engine.play(clip);
    require(static_cast<bool>(voice), "a valid clip must create a voice");
    require(engine.activeVoiceCount() == 1, "live voice accounting must include a created voice");
    require(engine.lengthFrames(voice) == 4, "voice length must use PCM frames");

    std::vector<float> first(4);
    require(engine.mix(first), "offline mix must fill a complete output buffer");
    require(near(first[0], 0.0f) && near(first[1], 0.0f) &&
            near(first[2], 0.25f) && near(first[3], 0.25f),
        "a newly scheduled voice must start on the next mixer frame");
    require(engine.positionFrames(voice) > 0, "mixing must advance the voice cursor");

    const std::uint64_t beforeSuspend = engine.positionFrames(voice);
    engine.suspend();
    require(engine.suspended(), "audio suspension state must be observable");
    std::vector<float> suspendedOutput(8, 1.0f);
    require(engine.mix(suspendedOutput), "a suspended mixer must return bounded silence");
    require(std::all_of(suspendedOutput.begin(), suspendedOutput.end(),
                [](float sample) { return near(sample, 0.0f); }) &&
            engine.positionFrames(voice) == beforeSuspend,
        "suspension must silence output without advancing active voices");
    engine.resume();
    require(!engine.suspended(), "audio resume must clear suspension state");

    require(engine.pause(voice), "pause must accept an active voice");
    require(engine.isPaused(voice), "paused voice must expose pause state");
    const std::uint64_t pausedAt = engine.positionFrames(voice);
    std::vector<float> silence(4, 1.0f);
    require(engine.mix(silence), "paused mix must remain operational");
    require(std::all_of(silence.begin(), silence.end(),
                [](float sample) { return near(sample, 0.0f); }),
        "paused voices must produce silence");
    require(engine.positionFrames(voice) == pausedAt, "pause must preserve position");

    require(engine.seekFrames(voice, 1) && engine.resume(voice),
        "seek and resume must operate on a paused voice");
    require(!engine.isPaused(voice), "resumed voice must clear pause state");
    std::vector<float> resumed(2);
    require(engine.mix(resumed), "resumed voice must mix");
    require(near(resumed[0], -0.5f) && near(resumed[1], -0.5f),
        "seek must select the requested PCM frame");

    require(engine.setVoiceVolume(voice, 0.5f), "volume update must succeed");
    require(engine.seekFrames(voice, 0), "voice must seek to its start");
    std::vector<float> quieter(2);
    require(engine.mix(quieter), "volume-adjusted voice must mix");
    std::vector<float> quieterNext(4);
    require(engine.mix(quieterNext), "continued volume-adjusted voice must mix");
    require(near(quieterNext[2], 0.125f) && near(quieterNext[3], 0.125f),
        "seek must settle within the mixer's bounded queued-frame latency and volume must scale output");

    require(engine.stop(voice), "stop must accept a valid voice");
    require(!engine.isPlaying(voice), "stopped voice must not report playing");
    require(!engine.isPaused(voice), "stopped voice must not report paused");
    require(!engine.destroyClip(clip),
        "clips referenced by a live voice must not be destroyed");
    require(engine.destroyVoice(voice), "stopped voices must be releasable");
    require(engine.activeVoiceCount() == 0, "live voice accounting must exclude destroyed voices");
    require(!engine.stop(voice), "stale voice handles must be rejected");
    require(engine.destroyClip(clip), "unreferenced clips must be releasable");
    require(!engine.destroyClip(clip), "stale clip handles must be rejected");

    Engine resampleEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle resampleClip = resampleEngine.createClip({
        .sampleRate = 22050,
        .channels = 1,
        .samples = std::vector<float>(22050, 0.25f),
    });
    const VoiceHandle resampleVoice = resampleEngine.play(resampleClip);
    std::vector<float> halfSecondAt48k(48000);
    require(resampleEngine.mix(halfSecondAt48k),
        "cross-rate PCM must resample into the engine output format");
    const std::uint64_t resampledCursor = resampleEngine.positionFrames(resampleVoice);
    require(resampledCursor > 10000 && resampledCursor < 12000 &&
            resampleEngine.isPlaying(resampleVoice),
        "22.05 kHz PCM must retain its one-second duration in a 48 kHz mixer");

    VoiceParameters halfPitchParameters;
    halfPitchParameters.pitch = 0.5f;
    const VoiceHandle halfPitchVoice = resampleEngine.play(resampleClip, halfPitchParameters);
    require(resampleEngine.mix(halfSecondAt48k),
        "half-speed cross-rate PCM must remain mixable");
    const std::uint64_t halfPitchCursor = resampleEngine.positionFrames(halfPitchVoice);
    require(halfPitchCursor > 5000 && halfPitchCursor < 6000 &&
            resampleEngine.isPlaying(halfPitchVoice),
        "PlaybackSpeed 0.5 must halve source-frame advance without changing output rate");

    ClipHandle replacement = engine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = {1.0f, 0.0f}});
    require(replacement.index == clip.index && replacement.generation != clip.generation,
        "reused clip slots must advance generation counters");
    require(engine.clipLengthFrames(replacement) == 2 &&
            engine.clipSampleRate(replacement) == 48000,
        "clip timing metadata must remain available before a voice is created");

    PcmClip sustained{
        .sampleRate = 48000,
        .channels = 1,
        .samples = std::vector<float>(2048, 0.25f),
    };
    Engine dryEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle dryClip = dryEngine.createClip(sustained);
    require(static_cast<bool>(dryEngine.play(dryClip)), "dry reference voice must start");
    std::vector<float> dryMix(512);
    require(dryEngine.mix(dryMix), "dry reference must mix");

    Engine spatialEngine({.sampleRate = 48000, .channels = 2});
    spatialEngine.setListener({});
    const ClipHandle spatialClip = spatialEngine.createClip(std::move(sustained));
    VoiceParameters spatialParameters;
    spatialParameters.spatial = true;
    spatialParameters.position = {10.0f, 0.0f, 0.0f};
    spatialParameters.minDistance = 1.0f;
    spatialParameters.maxDistance = 100.0f;
    spatialParameters.rolloff = 1.0f;
    require(static_cast<bool>(spatialEngine.play(spatialClip, spatialParameters)),
        "spatial voice must start");
    std::vector<float> spatialMix(512);
    require(spatialEngine.mix(spatialMix), "spatial voice must mix");
    const float dryEnergy = channelEnergy(dryMix, 0) + channelEnergy(dryMix, 1);
    const float spatialLeft = channelEnergy(spatialMix, 0);
    const float spatialRight = channelEnergy(spatialMix, 1);
    require(spatialLeft + spatialRight < dryEnergy,
        "distance attenuation must reduce spatial signal energy");
    require(std::abs(spatialLeft - spatialRight) > 0.01f,
        "off-axis sources must produce stereo panning");

    Engine linearEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle linearClip = linearEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = std::vector<float>(2048, 0.25f)});
    VoiceParameters linearParameters = spatialParameters;
    linearParameters.position = {100.0f, 0.0f, 0.0f};
    linearParameters.attenuation = AttenuationModel::Linear;
    require(static_cast<bool>(linearEngine.play(linearClip, linearParameters)),
        "linear attenuation voice must start");
    std::vector<float> linearMix(512);
    require(linearEngine.mix(linearMix), "linear attenuation voice must mix");
    require(channelEnergy(linearMix, 0) + channelEnergy(linearMix, 1) < 0.01f,
        "linear attenuation must reach silence at maximum distance");

    Engine curveEngine({.sampleRate = 48000, .channels = 2});
    curveEngine.setListener({});
    const ClipHandle curveClip = curveEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(4096, 0.25f)});
    VoiceParameters curveParameters;
    curveParameters.looping = true;
    curveParameters.spatial = true;
    curveParameters.position = {0.0f, 0.0f, -5.0f};
    const VoiceHandle curveVoice = curveEngine.play(curveClip, curveParameters);
    std::vector<float> curveStartup(128);
    require(curveEngine.mix(curveStartup),
        "custom-curve voices must initialize through the normal mixer path");
    const std::array<AttenuationPoint, 2> curve{{{0.0f, 1.0f}, {10.0f, 0.0f}}};
    require(curveEngine.setVoiceAttenuationCurve(curveVoice, curve),
        "a finite strictly ordered custom distance curve must be accepted");
    std::vector<float> curveMidpoint(512);
    for (unsigned block = 0; block < 16; ++block)
        require(curveEngine.mix(curveMidpoint),
            "custom attenuation playback must remain mixable");
    const float curveMidpointEnergy = channelEnergy(curveMidpoint, 0) +
        channelEnergy(curveMidpoint, 1);
    require(curveMidpointEnergy > 1.0f,
        "custom curves must interpolate gain at the listener distance");
    require(curveEngine.setVoiceTransform(curveVoice, {}, {}),
        "custom-curve source transforms must remain updateable");
    std::vector<float> curveNear(512);
    for (unsigned block = 0; block < 16; ++block)
        require(curveEngine.mix(curveNear),
            "near custom-curve playback must remain mixable");
    require(channelEnergy(curveNear, 0) + channelEnergy(curveNear, 1) >
                curveMidpointEnergy * 1.5f,
        "the first custom distance key must be louder than an interpolated midpoint");
    require(curveEngine.setVoiceTransform(curveVoice, {0.0f, 0.0f, -5.0f}, {}),
        "custom-curve source movement must update attenuation");
    ListenerState movedListener;
    movedListener.position = {0.0f, 0.0f, 5.0f};
    curveEngine.setListener(movedListener);
    std::vector<float> curveSilent(512);
    for (unsigned block = 0; block < 16; ++block)
        require(curveEngine.mix(curveSilent),
            "silent custom-curve playback must remain mixable");
    require(channelEnergy(curveSilent, 0) + channelEnergy(curveSilent, 1) < 0.01f,
        "listener movement must update custom distance attenuation to its final key");
    const std::array<AttenuationPoint, 2> invalidCurve{{{1.0f, 1.0f}, {1.0f, 0.0f}}};
    require(!curveEngine.setVoiceAttenuationCurve(curveVoice, invalidCurve),
        "custom distance curves must reject duplicate or unordered distances");

    Engine angleEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle angleClip = angleEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(4096, 0.25f)});
    VoiceParameters angleParameters;
    angleParameters.looping = true;
    angleParameters.spatial = true;
    angleParameters.position = {0.0f, 0.0f, -5.0f};
    angleParameters.direction = {0.0f, 0.0f, 1.0f};
    const VoiceHandle angleVoice = angleEngine.play(angleClip, angleParameters);
    const std::array<AttenuationPoint, 2> angleCurve{{
        {0.0f, 1.0f}, {180.0f, 0.0f}}};
    require(angleEngine.setVoiceAngleAttenuationCurve(angleVoice, angleCurve),
        "a finite ordered custom angle curve must be accepted");
    std::vector<float> angleFront(512);
    for (unsigned block = 0; block < 16; ++block)
        require(angleEngine.mix(angleFront),
            "front-facing custom angle playback must remain mixable");
    const float angleFrontEnergy = channelEnergy(angleFront, 0) +
        channelEnergy(angleFront, 1);
    require(angleFrontEnergy > 1.0f,
        "the zero-degree angle key must preserve front-facing audio");
    require(angleEngine.setVoiceDirection(angleVoice, {0.0f, 0.0f, -1.0f}),
        "a live emitter direction change must be accepted");
    std::vector<float> angleBack(512);
    for (unsigned block = 0; block < 16; ++block)
        require(angleEngine.mix(angleBack),
            "rear-facing custom angle playback must remain mixable");
    require(channelEnergy(angleBack, 0) + channelEnergy(angleBack, 1) < 0.01f,
        "the 180-degree angle key must silence rear-facing audio");
    const std::array<AttenuationPoint, 2> invalidAngle{{
        {0.0f, 1.0f}, {181.0f, 0.0f}}};
    require(!angleEngine.setVoiceAngleAttenuationCurve(angleVoice, invalidAngle),
        "custom angle curves must reject angles outside 0 to 180 degrees");

    Engine listenerCurveEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle listenerCurveClip = listenerCurveEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(2048, 0.25f)});
    VoiceParameters listenerCurveVoiceParameters;
    listenerCurveVoiceParameters.looping = true;
    listenerCurveVoiceParameters.spatial = true;
    listenerCurveVoiceParameters.position = {0.0f, 0.0f, -10.0f};
    require(static_cast<bool>(listenerCurveEngine.play(
        listenerCurveClip, listenerCurveVoiceParameters)),
        "listener-curve voice must start");
    ListenerState listenerCurveState;
    listenerCurveState.distanceAttenuationCurve = {{0.0f, 1.0f}, {10.0f, 0.0f}};
    listenerCurveEngine.setListener(listenerCurveState);
    std::vector<float> listenerCurveMix(512);
    for (unsigned block = 0; block < 16; ++block)
        require(listenerCurveEngine.mix(listenerCurveMix),
            "listener distance attenuation must remain mixable");
    require(channelEnergy(listenerCurveMix, 0) +
            channelEnergy(listenerCurveMix, 1) < 0.01f,
        "listener distance attenuation must silence its terminal key");

    Engine splitListenerEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle splitListenerClip = splitListenerEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(4096, 0.25f)});
    VoiceParameters graphVoiceParameters;
    graphVoiceParameters.looping = true;
    graphVoiceParameters.spatial = true;
    graphVoiceParameters.listenerIndex = 1;
    graphVoiceParameters.position = {0.0f, 0.0f, -10.0f};
    require(static_cast<bool>(splitListenerEngine.play(
        splitListenerClip, graphVoiceParameters)),
        "advanced graph voice must start on its dedicated listener");
    splitListenerEngine.setListener(listenerCurveState);
    splitListenerEngine.setGraphListener({});
    std::vector<float> graphListenerMix(512);
    for (unsigned block = 0; block < 16; ++block)
        require(splitListenerEngine.mix(graphListenerMix),
            "advanced graph listener must remain mixable");
    require(channelEnergy(graphListenerMix, 0) +
            channelEnergy(graphListenerMix, 1) > 1.0f,
        "legacy listener curves must not attenuate advanced graph voices");
    splitListenerEngine.setGraphListener(listenerCurveState);
    for (unsigned block = 0; block < 16; ++block)
        require(splitListenerEngine.mix(graphListenerMix),
            "advanced graph listener attenuation must remain mixable");
    require(channelEnergy(graphListenerMix, 0) +
            channelEnergy(graphListenerMix, 1) < 0.01f,
        "advanced graph listener must independently silence its voices");

    Engine independentEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle independentClip = independentEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = std::vector<float>(2048, 0.25f)});
    const VoiceHandle pausedVoice = independentEngine.play(independentClip);
    const VoiceHandle runningVoice = independentEngine.play(independentClip);
    require(independentEngine.pause(pausedVoice), "one of two shared-clip voices must pause");
    const std::uint64_t pausedCursor = independentEngine.positionFrames(pausedVoice);
    std::vector<float> independentMix(256);
    require(independentEngine.mix(independentMix) && channelEnergy(independentMix, 0) > 0.0f,
        "a second voice must continue when the first shared-clip voice pauses");
    require(independentEngine.positionFrames(pausedVoice) == pausedCursor &&
            independentEngine.positionFrames(runningVoice) > 0,
        "voices sharing a clip must retain independent cursors");

    Engine loopEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle loopClip = loopEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = {0.25f, -0.25f}});
    VoiceParameters loopParameters;
    loopParameters.looping = true;
    const VoiceHandle loopVoice = loopEngine.play(loopClip, loopParameters);
    std::vector<float> loopMix(128);
    require(loopEngine.mix(loopMix), "looping voice must mix beyond one clip duration");
    require(loopEngine.isPlaying(loopVoice) &&
            channelEnergy(loopMix, 0) + channelEnergy(loopMix, 1) > 1.0f,
        "looping voice must remain active and repeat audible PCM");

    bool rejected = false;
    try
    {
        static_cast<void>(engine.createClip({.sampleRate = 0, .channels = 1, .samples = {1.0f}}));
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    require(rejected, "invalid PCM metadata must be rejected");

    const std::vector<std::byte> wav = makeMonoPcm16Wav();
    Engine decodeEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle decoded = decodeEngine.createEncodedClip(wav);
    const VoiceHandle decodedVoice = decodeEngine.play(decoded);
    require(decodeEngine.lengthFrames(decodedVoice) == 4,
        "encoded WAV must preserve its decoded frame count");
    std::vector<float> decodedMix(8);
    require(decodeEngine.mix(decodedMix) && channelEnergy(decodedMix, 0) > 0.5f,
        "encoded WAV must produce audible deterministic PCM");

    const std::filesystem::path streamPath =
        std::filesystem::temp_directory_path() / "rbx-audio-stream-contract.wav";
    {
        std::ofstream streamFile(streamPath, std::ios::binary | std::ios::trunc);
        streamFile.write(reinterpret_cast<const char*>(wav.data()),
            static_cast<std::streamsize>(wav.size()));
    }
    Engine streamEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle streamClip = streamEngine.createStreamingClip(streamPath);
    require(streamEngine.clipLengthFrames(streamClip) == 4,
        "streaming clips must expose metadata without full predecode");
    const VoiceHandle streamVoice = streamEngine.play(streamClip);
    require(static_cast<bool>(streamVoice), "asynchronous streaming playback must schedule");
    bool streamAudible = false;
    for (unsigned attempt = 0; attempt < 100 && !streamAudible; ++attempt)
    {
        std::vector<float> streamMix(8);
        require(streamEngine.mix(streamMix), "streaming mixer reads must remain operational");
        streamAudible = channelEnergy(streamMix, 0) > 0.1f;
        if (!streamAudible)
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    require(streamAudible, "background streaming decode must reach the mixer within a bounded wait");
    require(streamEngine.destroyVoice(streamVoice) && streamEngine.destroyClip(streamClip),
        "stream resources must tear down cleanly");
    std::filesystem::remove(streamPath);

    rejected = false;
    try
    {
        const std::array<std::byte, 4> corrupt{};
        static_cast<void>(decodeEngine.createEncodedClip(corrupt));
    }
    catch (const std::runtime_error&)
    {
        rejected = true;
    }
    require(rejected, "corrupt encoded audio must fail with a bounded error");

    rejected = false;
    try
    {
        Engine limited({.sampleRate = 48000, .channels = 2,
            .maxClipFrames = 2, .maxEncodedBytes = 1024});
        static_cast<void>(limited.createEncodedClip(wav));
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    require(rejected, "decoded clips must obey configured frame limits");

    Engine boundedVoices({.sampleRate = 48000, .channels = 2,
        .maxClipFrames = 1024, .maxEncodedBytes = 1024, .maxVoices = 1});
    const ClipHandle boundedClip = boundedVoices.createClip({
        .sampleRate = 48000, .channels = 1, .samples = std::vector<float>(64, 0.25f)});
    VoiceParameters important;
    important.looping = true;
    important.priority = 10;
    const VoiceHandle firstImportant = boundedVoices.play(boundedClip, important);
    VoiceParameters disposable = important;
    disposable.priority = 0;
    require(!static_cast<bool>(boundedVoices.play(boundedClip, disposable)),
        "a lower-priority voice must not steal the only important voice");
    require(boundedVoices.isPlaying(firstImportant),
        "rejected low-priority playback must leave the current voice alive");
    const VoiceHandle replacementImportant = boundedVoices.play(boundedClip, important);
    require(replacementImportant.index == firstImportant.index &&
            replacementImportant.generation != firstImportant.generation,
        "equal-priority playback must deterministically steal the oldest voice");
    require(boundedVoices.activeVoiceCount() == 1,
        "voice stealing must preserve the configured live-voice bound");
    require(!boundedVoices.isPlaying(firstImportant) && boundedVoices.isPlaying(replacementImportant),
        "stolen handles must become stale immediately");

    Engine masterEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle masterClip = masterEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = std::vector<float>(512, 0.5f)});
    VoiceParameters masterLoop;
    masterLoop.looping = true;
    require(static_cast<bool>(masterEngine.play(masterClip, masterLoop)),
        "master-gain test voice must start");
    masterEngine.setMasterVolume(0.25f);
    require(near(masterEngine.masterVolume(), 0.25f), "master volume must be retained");
    std::vector<float> masterMix(64);
    require(masterEngine.mix(masterMix) && channelEnergy(masterMix, 0) < 4.0f,
        "master volume must attenuate mixed voices");
    masterEngine.setMuted(true);
    require(masterEngine.muted(), "mute state must be retained");
    std::fill(masterMix.begin(), masterMix.end(), 1.0f);
    require(masterEngine.mix(masterMix) && channelEnergy(masterMix, 0) == 0.0f &&
            channelEnergy(masterMix, 1) == 0.0f,
        "mute must silence the master output");
    masterEngine.setMuted(false);
    std::fill(masterMix.begin(), masterMix.end(), 0.0f);
    require(masterEngine.mix(masterMix) && channelEnergy(masterMix, 0) > 0.0f,
        "unmute must restore the retained master volume");

    Engine busEngine({.sampleRate = 48000, .channels = 2});
    const BusHandle quietBus = busEngine.createBus(0.25f);
    require(static_cast<bool>(quietBus) && near(busEngine.busVolume(quietBus), 0.25f),
        "a bus must retain its initial volume");
    const ClipHandle busClip = busEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = std::vector<float>(512, 0.5f)});
    VoiceParameters busParameters;
    busParameters.looping = true;
    busParameters.bus = quietBus;
    const VoiceHandle busVoice = busEngine.play(busClip, busParameters);
    require(static_cast<bool>(busVoice), "a voice must route through a valid bus");
    require(!busEngine.destroyBus(quietBus), "a bus with a live routed voice must be retained");
    std::vector<float> busMix(64);
    require(busEngine.mix(busMix) && channelEnergy(busMix, 0) < 4.0f,
        "bus gain must attenuate routed voices");
    require(busEngine.setBusVolume(quietBus, 0.0f), "bus volume updates must succeed");
    std::fill(busMix.begin(), busMix.end(), 1.0f);
    require(busEngine.mix(busMix) && channelEnergy(busMix, 0) == 0.0f,
        "a muted bus must silence routed voices");
    require(busEngine.destroyVoice(busVoice) && busEngine.destroyBus(quietBus),
        "an unused bus must be releasable");
    require(!busEngine.setBusVolume(quietBus, 1.0f),
        "stale bus handles must be rejected");

    Engine reverbEngine({.sampleRate = 48000, .channels = 2});
    std::vector<float> impulse(32, 0.0f);
    impulse[0] = 1.0f;
    const ClipHandle impulseClip = reverbEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = std::move(impulse)});
    require(static_cast<bool>(reverbEngine.play(impulseClip)), "reverb impulse must start");
    const ReverbParameters reverbParameters{
        .enabled = true, .mix = 0.5f, .decay = 0.7f, .damping = 0.2f, .roomSize = 0.5f};
    reverbEngine.setReverb(reverbParameters);
    require(reverbEngine.reverb().enabled && near(reverbEngine.reverb().mix, 0.5f),
        "reverb controls must retain their bounded values");
    std::vector<float> reverbMix(12000, 0.0f);
    require(reverbEngine.mix(reverbMix), "reverb impulse response must mix");
    float lateEnergy = 0.0f;
    for (std::size_t index = 2000; index < reverbMix.size(); ++index)
        lateEnergy += std::abs(reverbMix[index]);
    require(lateEnergy > 0.01f,
        "enabled reverb must generate a deterministic late tail after the dry clip ends");

    Engine echoEngine({.sampleRate = 48000, .channels = 2});
    std::vector<float> echoImpulse(8, 0.0f);
    echoImpulse[0] = 1.0f;
    const ClipHandle echoClip = echoEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = std::move(echoImpulse)});
    require(static_cast<bool>(echoEngine.play(echoClip)), "echo impulse must start");
    echoEngine.setEcho({.enabled = true, .delaySeconds = 0.001f, .feedback = 0.5f, .mix = 0.5f});
    require(echoEngine.echo().enabled && near(echoEngine.echo().delaySeconds, 0.001f),
        "echo controls must retain their bounded values");
    std::vector<float> echoMix(512, 0.0f);
    require(echoEngine.mix(echoMix), "echo impulse response must mix");
    float echoTail = 0.0f;
    for (std::size_t index = 80; index < echoMix.size(); ++index)
        echoTail += std::abs(echoMix[index]);
    require(echoTail > 0.01f, "enabled echo must generate delayed output");

    Engine effectEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle effectClip = effectEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = std::vector<float>(2048, 1.0f)});
    require(static_cast<bool>(effectEngine.play(effectClip)), "effect test voice must start");
    effectEngine.setEqualizer({.enabled = true, .lowGainDb = -12.0f,
        .midGainDb = -12.0f, .highGainDb = -12.0f});
    effectEngine.setCompressor({.enabled = true, .thresholdDb = -18.0f,
        .ratio = 8.0f, .attackSeconds = 0.0001f, .releaseSeconds = 0.1f});
    require(effectEngine.equalizer().enabled && effectEngine.compressor().enabled,
        "equalizer and compressor controls must remain exposed");
    std::vector<float> effectMix(512);
    require(effectEngine.mix(effectMix) && channelEnergy(effectMix, 0) < 128.0f,
        "equalizer and compressor must attenuate a sustained over-threshold signal");

    Engine distortionEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle distortionClip = distortionEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(512, 0.1f)});
    VoiceParameters distortionParameters;
    distortionParameters.looping = true;
    distortionParameters.distortionLevels[0] = 0.75f;
    distortionParameters.distortionCount = 1;
    const VoiceHandle distortionVoice = distortionEngine.play(
        distortionClip, distortionParameters);
    require(static_cast<bool>(distortionVoice),
        "a voice with a distortion node must start");
    std::vector<float> distortionMix(128);
    require(distortionEngine.mix(distortionMix) &&
            channelEnergy(distortionMix, 0) > 20.0f,
        "voice distortion must apply nonlinear drive before the output endpoint");
    require(distortionEngine.setVoiceDistortion(
                distortionVoice, std::span<const float>{}) &&
            distortionEngine.mix(distortionMix) &&
            channelEnergy(distortionMix, 0) < 8.0f,
        "bypassing live voice distortion must restore the dry signal");

    Engine tremoloEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle tremoloClip = tremoloEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(48000, 1.0f)});
    VoiceParameters tremoloParameters;
    tremoloParameters.looping = true;
    tremoloParameters.effects[0].type = VoiceEffectType::Tremolo;
    tremoloParameters.effects[0].parameters = {
        1.0f, 1.0f, 10.0f, 0.5f, 0.0f, 0.0f, 0.0f};
    tremoloParameters.effectCount = 1;
    const VoiceHandle tremoloVoice = tremoloEngine.play(
        tremoloClip, tremoloParameters);
    require(static_cast<bool>(tremoloVoice),
        "a voice with a tremolo node must start");
    std::vector<float> tremoloMix(4800 * 2);
    require(tremoloEngine.mix(tremoloMix) &&
            channelEnergy(tremoloMix, 0) > 400.0f &&
            channelEnergy(tremoloMix, 0) < 4200.0f,
        "voice tremolo must apply its sample-rate-correct periodic gain");
    VoiceEffect dryTremolo;
    dryTremolo.type = VoiceEffectType::Tremolo;
    dryTremolo.parameters = {0.0f, 1.0f, 10.0f, 0.5f, 0.0f, 0.0f, 0.0f};
    require(tremoloEngine.setVoiceEffects(tremoloVoice,
                std::span<const VoiceEffect>(&dryTremolo, 1)) &&
            tremoloEngine.mix(tremoloMix) &&
            channelEnergy(tremoloMix, 0) > 4700.0f,
        "a zero-depth live tremolo update must restore the dry voice");
    dryTremolo.parameters[2] = std::numeric_limits<float>::quiet_NaN();
    require(!tremoloEngine.setVoiceEffects(tremoloVoice,
                std::span<const VoiceEffect>(&dryTremolo, 1)),
        "voice effects must reject non-finite realtime parameters");

    for (const VoiceEffectType type : {
             VoiceEffectType::Chorus, VoiceEffectType::Flanger})
    {
        Engine modulationEngine({.sampleRate = 48000, .channels = 2});
        std::vector<float> modulationImpulse(4096, 0.0f);
        modulationImpulse[0] = 1.0f;
        const ClipHandle modulationClip = modulationEngine.createClip({
            .sampleRate = 48000, .channels = 1,
            .samples = std::move(modulationImpulse)});
        VoiceParameters modulationParameters;
        modulationParameters.effects[0].type = type;
        modulationParameters.effects[0].parameters = {
            0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
        modulationParameters.effectCount = 1;
        require(static_cast<bool>(modulationEngine.play(
                    modulationClip, modulationParameters)),
            "a voice with a modulation delay must start");
        std::vector<float> modulationMix(2048 * 2);
        require(modulationEngine.mix(modulationMix) &&
                std::abs(modulationMix[0]) < 0.001f &&
                channelEnergy(modulationMix, 0) > 0.5f,
            "chorus and flanger must produce their bounded delayed wet signal");
    }

    Engine graphCompressorEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle graphCompressorClip = graphCompressorEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(8192, 1.0f)});
    VoiceParameters graphCompressorParameters;
    graphCompressorParameters.effects[0].type = VoiceEffectType::Compressor;
    graphCompressorParameters.effects[0].parameters = {
        0.0001f, 0.0f, 10.0f, 0.1f, -20.0f, 0.0f, 0.0f};
    graphCompressorParameters.effectCount = 1;
    const VoiceHandle graphCompressorVoice = graphCompressorEngine.play(
        graphCompressorClip, graphCompressorParameters);
    require(static_cast<bool>(graphCompressorVoice),
        "a voice with a graph compressor must start");
    std::vector<float> graphCompressorMix(4096 * 2);
    require(graphCompressorEngine.mix(graphCompressorMix) &&
            channelEnergy(graphCompressorMix, 0) < 1500.0f,
        "the per-voice graph compressor must reduce an over-threshold signal");
    require(graphCompressorEngine.setVoiceEffects(graphCompressorVoice, {}) &&
            graphCompressorEngine.mix(graphCompressorMix) &&
            channelEnergy(graphCompressorMix, 0) > 4000.0f,
        "bypassing the graph compressor must restore the dry voice");

    Engine gateEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle gateClip = gateEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(4096, 0.01f)});
    VoiceParameters gateParameters;
    gateParameters.effects[0].type = VoiceEffectType::Gate;
    gateParameters.effects[0].parameters = {
        0.001f, 0.001f, -30.0f, -20.0f, 0.0f, 0.0f, 0.0f};
    gateParameters.effectCount = 1;
    require(static_cast<bool>(gateEngine.play(gateClip, gateParameters)),
        "a voice with a graph gate must start");
    std::vector<float> dynamicsMix(1024 * 2);
    require(gateEngine.mix(dynamicsMix) &&
            channelEnergy(dynamicsMix, 0) < 0.01f,
        "the graph gate must close below its hysteresis range");

    Engine limiterEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle limiterClip = limiterEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(4096, 2.0f)});
    VoiceParameters limiterParameters;
    limiterParameters.effects[0].type = VoiceEffectType::Limiter;
    limiterParameters.effects[0].parameters = {
        -6.0f, 0.1f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    limiterParameters.effectCount = 1;
    require(static_cast<bool>(limiterEngine.play(limiterClip,
                limiterParameters)),
        "a voice with a graph limiter must start");
    require(limiterEngine.mix(dynamicsMix) &&
            *std::max_element(dynamicsMix.begin(), dynamicsMix.end()) < 0.51f,
        "the graph limiter must enforce its authored ceiling");

    Engine graphEqualizerEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle graphEqualizerClip = graphEqualizerEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(16384, 1.0f)});
    VoiceParameters graphEqualizerParameters;
    graphEqualizerParameters.effects[0].type = VoiceEffectType::Equalizer;
    graphEqualizerParameters.effects[0].parameters = {
        -20.0f, 0.0f, 0.0f, 400.0f, 4000.0f, 0.0f, 0.0f};
    graphEqualizerParameters.effectCount = 1;
    require(static_cast<bool>(graphEqualizerEngine.play(graphEqualizerClip,
                graphEqualizerParameters)),
        "a voice with a graph equalizer must start");
    std::vector<float> equalizerMix(8192 * 2);
    require(graphEqualizerEngine.mix(equalizerMix) &&
            channelEnergy(equalizerMix, 0) < 1200.0f,
        "the graph equalizer must attenuate its authored low band");

    Engine graphFilterEngine({.sampleRate = 48000, .channels = 2});
    std::vector<float> filterImpulse(16384);
    for (std::size_t index = 0; index < filterImpulse.size(); ++index)
        filterImpulse[index] = index % 2 ? -1.0f : 1.0f;
    const ClipHandle graphFilterClip = graphFilterEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = filterImpulse});
    VoiceParameters graphFilterParameters;
    graphFilterParameters.effects[0].type = VoiceEffectType::Filter;
    graphFilterParameters.effects[0].parameters = {
        4.0f, 1000.0f, 0.0f, 0.70710678f, 0.0f, 0.0f, 0.0f};
    graphFilterParameters.effectCount = 1;
    require(static_cast<bool>(graphFilterEngine.play(graphFilterClip,
                graphFilterParameters)),
        "a voice with a graph filter must start");
    std::vector<float> filterMix(8192 * 2);
    require(graphFilterEngine.mix(filterMix) &&
            channelEnergy(filterMix, 0) < 10.0f,
        "the graph low-pass filter must apply its authored cascade");

    Engine graphPitchEngine({.sampleRate = 48000, .channels = 2});
    std::vector<float> pitchTone(32768);
    for (std::size_t index = 0; index < pitchTone.size(); ++index)
        pitchTone[index] = std::sin(6.2831853071795864769f * 440.0f *
            static_cast<float>(index) / 48000.0f);
    const ClipHandle graphPitchClip = graphPitchEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = pitchTone});
    VoiceParameters graphPitchParameters;
    graphPitchParameters.effects[0].type = VoiceEffectType::PitchShifter;
    graphPitchParameters.effects[0].parameters = {
        1.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    graphPitchParameters.effectCount = 1;
    require(static_cast<bool>(graphPitchEngine.play(graphPitchClip,
                graphPitchParameters)),
        "a voice with a graph pitch shifter must start");
    std::vector<float> pitchMix(8192 * 2);
    require(graphPitchEngine.mix(pitchMix) &&
            channelEnergy(pitchMix, 0) > 100.0f,
        "the graph pitch shifter must produce its windowed wet signal");

    Engine graphEchoEngine({.sampleRate = 48000, .channels = 2});
    std::vector<float> graphEchoImpulse(4096);
    graphEchoImpulse[0] = 1.0f;
    const ClipHandle graphEchoClip = graphEchoEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = graphEchoImpulse});
    VoiceParameters graphEchoParameters;
    graphEchoParameters.effects[0].type = VoiceEffectType::Echo;
    graphEchoParameters.effects[0].parameters = {
        0.01f, -80.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    graphEchoParameters.effectCount = 1;
    require(static_cast<bool>(graphEchoEngine.play(graphEchoClip,
                graphEchoParameters)),
        "a voice with a graph echo must start");
    std::vector<float> graphEchoMix(1024 * 2);
    require(graphEchoEngine.mix(graphEchoMix),
        "the graph echo mix must render");
    float graphEchoPeak = 0.0f;
    for (std::size_t frame = 400; frame < 600; ++frame)
        graphEchoPeak = std::max(graphEchoPeak,
            std::abs(graphEchoMix[frame * 2]));
    require(graphEchoPeak > 0.5f &&
            std::abs(graphEchoMix[0]) < 0.001f,
        "the graph echo must emit its authored delayed wet signal");

    Engine graphReverbEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle graphReverbClip = graphReverbEngine.createClip({
        .sampleRate = 48000, .channels = 1, .samples = graphEchoImpulse});
    VoiceParameters graphReverbParameters;
    graphReverbParameters.effects[0].type = VoiceEffectType::Reverb;
    graphReverbParameters.effects[0].parameters = {0.5f, 1.5f, 1.0f,
        1.0f, -80.0f, 0.01f, 20000.0f, 0.0f, 250.0f, 0.0f,
        5000.0f, 0.0f, 2.0f};
    graphReverbParameters.effectCount = 1;
    require(static_cast<bool>(graphReverbEngine.play(graphReverbClip,
                graphReverbParameters)),
        "a voice with graph reverb must start");
    std::vector<float> graphReverbMix(4096 * 2);
    require(graphReverbEngine.mix(graphReverbMix),
        "the graph reverb mix must render");
    float graphReverbTail = 0.0f;
    for (std::size_t frame = 400; frame < 4000; ++frame)
        graphReverbTail += std::abs(graphReverbMix[frame * 2]);
    require(graphReverbTail > 1.0f && std::abs(graphReverbMix[0]) < 0.001f,
        "the graph reverb must produce a delayed diffuse tail");

    Engine queuedEngine({.sampleRate = 48000, .channels = 2});
    require(queuedEngine.mixerTimeSeconds() == 0.0,
        "a fresh mixer clock must begin at zero");
    const ClipHandle queuedClip = queuedEngine.createQueuedClip(48000, 2, 48000, 256);
    require(queuedEngine.queuedFrameCapacity(queuedClip) == 256 &&
            queuedEngine.queuedFramesAvailable(queuedClip) == 0,
        "queued clips must expose their bounded ring capacity");
    std::vector<float> queuedSamples(128 * 2, 0.25f);
    require(queuedEngine.submitQueuedFrames(queuedClip, queuedSamples) == 128 &&
            queuedEngine.queuedFramesAvailable(queuedClip) == 128,
        "queued clips must accept complete interleaved frame submissions");
    const VoiceHandle queuedVoice = queuedEngine.play(queuedClip);
    require(static_cast<bool>(queuedVoice), "queued audio playback must start");
    std::vector<float> queuedMix(64 * 2);
    require(queuedEngine.mix(queuedMix) && channelEnergy(queuedMix, 0) > 1.0f,
        "queued audio must reach the shared mixer");
    require(std::abs(queuedEngine.mixerTimeSeconds() - 64.0 / 48000.0) < 1e-9,
        "the mixer clock must advance sample-accurately with offline output");
    require(queuedEngine.queuedFramesAvailable(queuedClip) < 128,
        "mixing queued audio must consume ring-buffer frames");
    require(queuedEngine.destroyVoice(queuedVoice) && queuedEngine.resetQueuedClip(queuedClip) &&
            queuedEngine.queuedFramesAvailable(queuedClip) == 0 &&
            queuedEngine.destroyClip(queuedClip),
        "queued audio must support stop, seek reset, and deterministic release");

    Engine scheduledEngine({.sampleRate = 48000, .channels = 2});
    const ClipHandle scheduledClip = scheduledEngine.createClip({
        .sampleRate = 48000, .channels = 1,
        .samples = std::vector<float>(1024, 0.5f)});
    VoiceParameters scheduledParameters;
    scheduledParameters.startMixerTimeSeconds = 128.0 / 48000.0;
    const VoiceHandle scheduledVoice = scheduledEngine.play(
        scheduledClip, scheduledParameters);
    std::vector<float> scheduledMix(64 * 2);
    require(scheduledEngine.mix(scheduledMix) &&
            channelEnergy(scheduledMix, 0) == 0.0f,
        "a timestamped voice must remain silent before its mixer deadline");
    require(scheduledEngine.mix(scheduledMix) &&
            channelEnergy(scheduledMix, 0) == 0.0f,
        "a timestamped voice must remain sample-accurately silent through its deadline");
    require(scheduledEngine.mix(scheduledMix) &&
            channelEnergy(scheduledMix, 0) > 1.0f,
        "a timestamped voice must start on the requested mixer frame");
    const double cancelledStop = scheduledEngine.mixerTimeSeconds() +
        64.0 / 48000.0;
    require(scheduledEngine.scheduleVoiceStop(scheduledVoice, cancelledStop) &&
            scheduledEngine.cancelVoiceStop(scheduledVoice),
        "a pending timestamped stop must be cancellable");
    require(scheduledEngine.mix(scheduledMix) &&
            scheduledEngine.mix(scheduledMix) &&
            channelEnergy(scheduledMix, 0) > 1.0f,
        "cancelling a timestamped stop must preserve playback");
    const double committedStop = scheduledEngine.mixerTimeSeconds() +
        64.0 / 48000.0;
    require(scheduledEngine.scheduleVoiceStop(scheduledVoice, committedStop) &&
            scheduledEngine.mix(scheduledMix) &&
            scheduledEngine.mix(scheduledMix) &&
            channelEnergy(scheduledMix, 0) == 0.0f,
        "a timestamped stop must silence the voice on the requested mixer frame");
    return 0;
}
