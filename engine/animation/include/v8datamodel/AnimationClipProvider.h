#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "Util/ContentId.h"

namespace RBX {

extern const char* const sAnimationClipProvider;

// AnimationClipProvider is the current public animation-asset service.  This
// engine's executable clip representation is KeyframeSequence, so the service
// deliberately shares the same loader and content cache as
// KeyframeSequenceProvider instead of maintaining a second animation pipeline.
class AnimationClipProvider
    : public DescribedNonCreatable<AnimationClipProvider, Instance,
          sAnimationClipProvider, Reflection::ClassDescriptor::RUNTIME_LOCAL>
    , public Service
{
public:
    AnimationClipProvider();

    ContentId registerActiveAnimationClip(shared_ptr<Instance> animationClip);
    ContentId registerAnimationClip(shared_ptr<Instance> animationClip);
    shared_ptr<Instance> getAnimationClip(ContentId assetId);
    shared_ptr<Instance> getAnimationClipById(int assetId, bool useCache);
    void getAnimationClipAsync(ContentId assetId,
        boost::function<void(shared_ptr<Instance>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
};

}
