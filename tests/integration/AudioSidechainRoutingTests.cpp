#include "V8DataModel/DataModel.h"
#include "V8DataModel/FactoryRegistration.h"
#include "V8DataModel/Folder.h"
#include "V8Tree/Verb.h"
#include "audio/SoundGroups.h"
#include "audio/SoundService.h"

#include <array>
#include <mutex>
#include <stdexcept>

int main()
{
    static std::once_flag registration;
    std::call_once(registration,
        [] { static RBX::FactoryRegistrator registrator; });
    RBX::Soundscape::SoundService::outputDeviceDisabled = true;
    boost::shared_ptr<RBX::DataModel> dataModel =
        RBX::DataModel::createDataModel(
            true, new RBX::NullVerb(nullptr, ""), false);
    RBX::Soundscape::SoundService* service =
        RBX::ServiceProvider::create<RBX::Soundscape::SoundService>(
            dataModel.get());
    boost::shared_ptr<RBX::Folder> root =
        RBX::Creatable<RBX::Instance>::create<RBX::Folder>();
    root->setParent(dataModel.get());
    boost::shared_ptr<RBX::Soundscape::SoundChannel> source =
        RBX::Creatable<RBX::Instance>::create<
            RBX::Soundscape::SoundChannel>();
    source->setParent(root.get());
    boost::shared_ptr<RBX::Soundscape::SoundChannel> target =
        RBX::Creatable<RBX::Instance>::create<
            RBX::Soundscape::SoundChannel>();
    target->setParent(root.get());
    boost::shared_ptr<RBX::Soundscape::CompressorSoundEffect> compressor =
        RBX::Creatable<RBX::Instance>::create<
            RBX::Soundscape::CompressorSoundEffect>();
    compressor->setSideChain(source.get());
    compressor->setParent(target.get());

    service->step(RBX::Time::Interval(0.0));
    std::array<RBX::Audio::VoiceEffect, 32> sourceEffects{};
    std::array<RBX::Audio::VoiceEffect, 32> targetEffects{};
    if (service->collectRuntimeSoundEffects(source.get(), sourceEffects) != 1 ||
        service->collectRuntimeSoundEffects(target.get(), targetEffects) != 1 ||
        sourceEffects[0].type != RBX::Audio::VoiceEffectType::Analyzer ||
        targetEffects[0].type != RBX::Audio::VoiceEffectType::Compressor ||
        !sourceEffects[0].meter ||
        sourceEffects[0].meter != targetEffects[0].meter)
        throw std::runtime_error(
            "external Sound sidechain detector routing failed");

    compressor->setEnabled(false);
    service->step(RBX::Time::Interval(0.0));
    const std::uint32_t disabledSourceCount =
        service->collectRuntimeSoundEffects(source.get(), sourceEffects);
    const std::uint32_t disabledTargetCount =
        service->collectRuntimeSoundEffects(target.get(), targetEffects);
    if (disabledSourceCount != 0 || disabledTargetCount != 0)
        throw std::runtime_error(
            "disabled compressor retained its sidechain detector");
    compressor->setEnabled(true);
    service->step(RBX::Time::Interval(0.0));

    boost::shared_ptr<RBX::Soundscape::SoundGroup> sourceGroup =
        RBX::Creatable<RBX::Instance>::create<RBX::Soundscape::SoundGroup>();
    sourceGroup->setParent(root.get());
    compressor->setSideChain(sourceGroup.get());
    service->step(RBX::Time::Interval(0.0));
    std::array<RBX::Audio::VoiceEffect, 32> groupEffects{};
    service->collectRuntimeSoundEffects(target.get(), targetEffects);
    if (service->collectRuntimeSoundEffects(sourceGroup.get(), groupEffects) !=
            1 ||
        groupEffects[0].type != RBX::Audio::VoiceEffectType::Analyzer ||
        !groupEffects[0].meter ||
        groupEffects[0].meter != targetEffects[0].meter)
        throw std::runtime_error(
            "external SoundGroup sidechain detector routing failed");
    compressor->setSideChain(source.get());

    boost::shared_ptr<RBX::Folder> invalidSource =
        RBX::Creatable<RBX::Instance>::create<RBX::Folder>();
    invalidSource->setParent(root.get());
    compressor->setSideChain(invalidSource.get());
    service->step(RBX::Time::Interval(0.0));
    service->collectRuntimeSoundEffects(target.get(), targetEffects);
    if (targetEffects[0].meter)
        throw std::runtime_error("invalid sidechain target remained active");
    compressor->setSideChain(source.get());

    boost::shared_ptr<RBX::Soundscape::CompressorSoundEffect> reverse =
        RBX::Creatable<RBX::Instance>::create<
            RBX::Soundscape::CompressorSoundEffect>();
    reverse->setSideChain(target.get());
    reverse->setParent(source.get());
    service->step(RBX::Time::Interval(0.0));
    service->collectRuntimeSoundEffects(source.get(), sourceEffects);
    service->collectRuntimeSoundEffects(target.get(), targetEffects);
    if (sourceEffects[0].meter || targetEffects[0].meter)
        throw std::runtime_error("cyclic sidechain routing remained active");

    reverse->setEnabled(false);
    service->step(RBX::Time::Interval(0.0));
    service->collectRuntimeSoundEffects(source.get(), sourceEffects);
    service->collectRuntimeSoundEffects(target.get(), targetEffects);
    if (sourceEffects[0].type != RBX::Audio::VoiceEffectType::Analyzer ||
        !targetEffects[0].meter)
        throw std::runtime_error(
            "disabling a cyclic sidechain did not restore routing");

    compressor->setSideChain(target.get());
    service->step(RBX::Time::Interval(0.0));
    service->collectRuntimeSoundEffects(target.get(), targetEffects);
    if (targetEffects[0].meter)
        throw std::runtime_error("self-sidechain routing remained active");

    compressor->setSideChain(source.get());
    source->setParent(nullptr);
    service->step(RBX::Time::Interval(0.0));
    service->collectRuntimeSoundEffects(target.get(), targetEffects);
    if (targetEffects[0].meter)
        throw std::runtime_error(
            "unavailable sidechain routing remained active");

    source->destroy();
    reverse.reset();
    source.reset();
    service->step(RBX::Time::Interval(0.0));
    service->collectRuntimeSoundEffects(target.get(), targetEffects);
    if (targetEffects[0].meter)
        throw std::runtime_error(
            "destroyed sidechain target retained detector state");

    root->setParent(nullptr);
    return 0;
}
