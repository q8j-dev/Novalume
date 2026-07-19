#include "V8DataModel/AnimationClipProvider.h"

#include "V8DataModel/KeyframeSequenceProvider.h"

namespace RBX {

const char* const sAnimationClipProvider = "AnimationClipProvider";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<AnimationClipProvider, ContentId(shared_ptr<Instance>)>
    funcRegisterActiveAnimationClip(&AnimationClipProvider::registerActiveAnimationClip,
        "RegisterActiveAnimationClip", "animationClip", Security::None);
static Reflection::BoundFuncDesc<AnimationClipProvider, ContentId(shared_ptr<Instance>)>
    funcRegisterAnimationClip(&AnimationClipProvider::registerAnimationClip,
        "RegisterAnimationClip", "animationClip", Security::None);
static Reflection::BoundFuncDesc<AnimationClipProvider, shared_ptr<Instance>(ContentId)>
    funcGetAnimationClip(&AnimationClipProvider::getAnimationClip,
        "GetAnimationClip", "assetId", Security::Plugin);
static Reflection::BoundFuncDesc<AnimationClipProvider, shared_ptr<Instance>(int, bool)>
    funcGetAnimationClipById(&AnimationClipProvider::getAnimationClipById,
        "GetAnimationClipById", "assetId", "useCache", Security::Plugin);
static Reflection::BoundYieldFuncDesc<AnimationClipProvider, shared_ptr<Instance>(ContentId)>
    funcGetAnimationClipAsync(&AnimationClipProvider::getAnimationClipAsync,
        "GetAnimationClipAsync", "assetId", Security::None);
REFLECTION_END();

AnimationClipProvider::AnimationClipProvider()
{
    setName(sAnimationClipProvider);
}

ContentId AnimationClipProvider::registerActiveAnimationClip(shared_ptr<Instance> animationClip)
{
    return ServiceProvider::create<KeyframeSequenceProvider>(this)
        ->registerActiveKeyframeSequence(animationClip);
}

ContentId AnimationClipProvider::registerAnimationClip(shared_ptr<Instance> animationClip)
{
    return ServiceProvider::create<KeyframeSequenceProvider>(this)
        ->registerKeyframeSequence(animationClip);
}

shared_ptr<Instance> AnimationClipProvider::getAnimationClip(ContentId assetId)
{
    return ServiceProvider::create<KeyframeSequenceProvider>(this)
        ->getKeyframeSequenceLua(assetId);
}

shared_ptr<Instance> AnimationClipProvider::getAnimationClipById(int assetId, bool useCache)
{
    return ServiceProvider::create<KeyframeSequenceProvider>(this)
        ->getKeyframeSequenceByIdLua(assetId, useCache);
}

void AnimationClipProvider::getAnimationClipAsync(ContentId assetId,
    boost::function<void(shared_ptr<Instance>)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    try
    {
        resumeFunction(getAnimationClip(assetId));
    }
    catch (const std::exception& exception)
    {
        errorFunction(exception.what());
    }
}

}
