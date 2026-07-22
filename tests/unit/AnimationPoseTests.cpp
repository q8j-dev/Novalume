#include "v8datamodel/IAnimatableJoint.h"
#include "v8datamodel/Animation.h"
#include "v8datamodel/AnimationTrack.h"
#include "v8datamodel/AnimationTrackState.h"
#include "v8datamodel/Animator.h"
#include "v8datamodel/Keyframe.h"
#include "v8datamodel/KeyframeSequence.h"
#include "v8datamodel/Pose.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool near(float left, float right)
{
    return std::abs(left - right) < 0.001f;
}

class RecordingJoint final : public RBX::IAnimatableJoint
{
public:
    RecordingJoint(std::string parentName, std::string partName)
        : parentName(std::move(parentName))
        , partName(std::move(partName))
    {
    }

    const std::string& getParentName() override
    {
        return parentName;
    }

    const std::string& getPartName() override
    {
        return partName;
    }

    void applyPose(const RBX::CachedPose& pose) override
    {
        appliedPose = pose;
        ++applyCount;
    }

    RBX::CachedPose appliedPose;
    int applyCount = 0;

private:
    std::string parentName;
    std::string partName;
};

boost::shared_ptr<RBX::Pose> makeLimbPose(float x)
{
    boost::shared_ptr<RBX::Pose> torso =
        RBX::Creatable<RBX::Instance>::create<RBX::Pose>();
    torso->setName("Torso");

    boost::shared_ptr<RBX::Pose> arm =
        RBX::Creatable<RBX::Instance>::create<RBX::Pose>();
    arm->setName("Right Arm");
    arm->setCoordinateFrame(RBX::CoordinateFrame(RBX::Vector3(x, 0.0f, 0.0f)));
    arm->setParent(torso.get());
    return torso;
}

boost::shared_ptr<RBX::Keyframe> makeKeyframe(
    float time, float armTranslation, const char* name = "Keyframe")
{
    boost::shared_ptr<RBX::Keyframe> keyframe =
        RBX::Creatable<RBX::Instance>::create<RBX::Keyframe>();
    keyframe->setTime(time);
    keyframe->setName(name);
    keyframe->addPose(makeLimbPose(armTranslation));
    return keyframe;
}

void incrementCounter(int* value)
{
    ++*value;
}

void recordKeyframe(std::vector<std::string>* names, const std::string& name)
{
    names->push_back(name);
}
}

int main()
{
    using namespace RBX;

    boost::shared_ptr<KeyframeSequence> sequence =
        Creatable<Instance>::create<KeyframeSequence>();
    sequence->setLoop(true);
    sequence->addKeyframe(makeKeyframe(0.0f, 0.0f, "Start"));
    sequence->addKeyframe(makeKeyframe(0.5f, 5.0f, "Middle"));
    sequence->addKeyframe(makeKeyframe(1.0f, 10.0f, "End"));

    RecordingJoint joint("Torso", "Right Arm");
    std::vector<PoseAccumulator> poses;
    poses.emplace_back(JointPair(sequence, &joint));

    sequence->apply(poses, 0.0, 0.5, 1.0f);
    require(near(poses[0].pose.translation.x, 5.0f),
        "animation sampling must interpolate the limb pose between keyframes");

    poses[0].pose = CachedPose();
    sequence->apply(poses, 0.5, 1.25, 1.0f);
    require(near(poses[0].pose.translation.x, 2.5f),
        "looping animation sampling must wrap time past its duration");

    poses[0].pose = CachedPose();
    sequence->apply(poses, 0.0, -0.25, 1.0f);
    require(near(poses[0].pose.translation.x, 7.5f),
        "reverse animation sampling must wrap negative time consistently");

    poses[0].pose = CachedPose();
    sequence->apply(poses, 0.0, 0.5, 0.5f);
    require(near(poses[0].pose.translation.x, 2.5f) &&
            near(poses[0].pose.weight, 0.5f),
        "track fade weight must scale the sampled transform and its accumulated influence");

    sequence->setLoop(false);
    poses[0].pose = CachedPose();
    sequence->apply(poses, 0.5, 2.0, 1.0f);
    require(near(poses[0].pose.translation.x, 10.0f),
        "a non-looping animation must hold its final authored pose after completion");

    sequence->setLoop(true);
    boost::shared_ptr<Animator> animator = Creatable<Instance>::create<Animator>();
    boost::shared_ptr<Animation> animation = Creatable<Instance>::create<Animation>();
    boost::shared_ptr<AnimationTrackState> trackState =
        Creatable<Instance>::create<AnimationTrackState>(sequence, animator);
    boost::shared_ptr<AnimationTrack> track =
        Creatable<Instance>::create<AnimationTrack>(trackState, animator, animation);
    require(track->getLooped(),
        "AnimationTrack must inherit the authored KeyframeSequence loop state");
    track->setLooped(false);
    require(!track->getLooped() &&
            track->findPropertyDescriptor("Looped") != nullptr &&
            track->findPropertyDescriptor("Speed") != nullptr &&
            track->findPropertyDescriptor("WeightCurrent") != nullptr &&
            track->findPropertyDescriptor("WeightTarget") != nullptr &&
            track->getDescriptor().findEventDescriptor("DidLoop") != nullptr,
        "AnimationTrack must expose current loop, speed, weight, and DidLoop contracts");

    // Drive the replicated track-state signals with explicit clock values so
    // the same fade behavior used when an action-priority emote replaces a
    // movement track is deterministic rather than wall-clock dependent.
    trackState->setPriority(KeyframeSequence::ACTION);
    trackState->internalPlaySignal(10.0f, 0.4f, 1.0f, 1.0f);
    require(trackState->getIsPlaying() &&
            near(static_cast<float>(trackState->getWeightAtTime(10.0)), 0.0f) &&
            near(static_cast<float>(trackState->getWeightAtTime(10.2)), 0.5f) &&
            near(static_cast<float>(trackState->getWeightAtTime(10.4)), 1.0f),
        "an emote track must fade from zero to its requested action weight");

    trackState->internalAdjustWeightSignal(10.4f, 0.25f, 0.2f);
    require(near(static_cast<float>(trackState->getWeightAtTime(10.5)), 0.625f) &&
            near(static_cast<float>(trackState->getWeightAtTime(10.6)), 0.25f),
        "an active emote must preserve the current pose weight while retargeting its fade");

    trackState->internalStopSignal(10.6f, 0.2f);
    require(!trackState->isStopped(10.79) && trackState->isStopped(10.81) &&
            near(static_cast<float>(trackState->getWeightAtTime(10.7)), 0.125f) &&
            near(static_cast<float>(trackState->getWeightAtTime(10.81)), 0.0f),
        "stopping an emote must retain its action pose until the authored fade completes");

    boost::shared_ptr<AnimationTrackState> hitchState =
        Creatable<Instance>::create<AnimationTrackState>(sequence, animator);
    int forwardLoops = 0;
    std::vector<std::string> forwardKeyframes;
    rbx::signals::scoped_connection forwardLoopConnection =
        hitchState->didLoopSignal.connect(boost::bind(&incrementCounter, &forwardLoops));
    rbx::signals::scoped_connection forwardKeyframeConnection =
        hitchState->keyframeReachedSignal.connect(
            boost::bind(&recordKeyframe, &forwardKeyframes, _1));
    hitchState->internalPlaySignal(20.0f, 0.0f, 1.0f, 1.0f);
    poses[0].pose = CachedPose();
    hitchState->step(poses, 23.5);
    require(forwardLoops == 3 &&
            std::count(forwardKeyframes.begin(), forwardKeyframes.end(), "Middle") == 4 &&
            near(poses[0].pose.translation.x, 5.0f),
        "a large forward hitch must report every loop/keyframe and sample the final phase");

    boost::shared_ptr<AnimationTrackState> reverseState =
        Creatable<Instance>::create<AnimationTrackState>(sequence, animator);
    int reverseLoops = 0;
    std::vector<std::string> reverseKeyframes;
    rbx::signals::scoped_connection reverseLoopConnection =
        reverseState->didLoopSignal.connect(boost::bind(&incrementCounter, &reverseLoops));
    rbx::signals::scoped_connection reverseKeyframeConnection =
        reverseState->keyframeReachedSignal.connect(
            boost::bind(&recordKeyframe, &reverseKeyframes, _1));
    reverseState->internalPlaySignal(30.0f, 0.0f, 1.0f, -1.0f);
    poses[0].pose = CachedPose();
    reverseState->step(poses, 33.5);
    require(reverseLoops == 3 &&
            std::count(reverseKeyframes.begin(), reverseKeyframes.end(), "Middle") == 4 &&
            near(poses[0].pose.translation.x, 5.0f),
        "a large reverse hitch must report every loop/keyframe and sample the final phase");

    reverseState->internalAdjustSpeedSignal(33.5f, 0.0f);
    const double pausedPosition = reverseState->getKeyframeAtTime(33.5);
    require(near(static_cast<float>(reverseState->getKeyframeAtTime(100.0)),
                 static_cast<float>(pausedPosition)),
        "zero animation speed must preserve the current phase deterministically");

    sequence->setLoop(false);
    boost::shared_ptr<AnimationTrackState> nonLoopingState =
        Creatable<Instance>::create<AnimationTrackState>(sequence, animator);
    std::vector<std::string> nonLoopingKeyframes;
    rbx::signals::scoped_connection nonLoopingConnection =
        nonLoopingState->keyframeReachedSignal.connect(
            boost::bind(&recordKeyframe, &nonLoopingKeyframes, _1));
    nonLoopingState->internalPlaySignal(40.0f, 0.0f, 1.0f, 1.0f);
    poses[0].pose = CachedPose();
    nonLoopingState->step(poses, 42.5);
    require(nonLoopingKeyframes.size() == 3 &&
            nonLoopingKeyframes[0] == "Start" &&
            nonLoopingKeyframes[1] == "Middle" &&
            nonLoopingKeyframes[2] == "End" &&
            !nonLoopingState->getIsPlaying() &&
            near(poses[0].pose.translation.x, 10.0f),
        "a non-looping hitch must deliver every crossed keyframe before stopping");

    boost::shared_ptr<KeyframeSequence> zeroDuration =
        Creatable<Instance>::create<KeyframeSequence>();
    zeroDuration->setLoop(false);
    zeroDuration->addKeyframe(makeKeyframe(0.0f, 3.0f, "Only"));
    boost::shared_ptr<AnimationTrackState> zeroDurationState =
        Creatable<Instance>::create<AnimationTrackState>(zeroDuration, animator);
    std::vector<std::string> zeroDurationKeyframes;
    rbx::signals::scoped_connection zeroDurationConnection =
        zeroDurationState->keyframeReachedSignal.connect(
            boost::bind(&recordKeyframe, &zeroDurationKeyframes, _1));
    zeroDurationState->internalPlaySignal(50.0f, 0.0f, 1.0f, 1.0f);
    poses[0].pose = CachedPose();
    zeroDurationState->step(poses, 50.1);
    require(zeroDurationKeyframes.size() == 1 &&
            zeroDurationKeyframes.front() == "Only" &&
            !zeroDurationState->getIsPlaying() &&
            near(poses[0].pose.translation.x, 3.0f),
        "a zero-duration sequence must apply its authored pose and stop deterministically");

    std::cout << "animation pose contract tests passed\n";
    return 0;
}
