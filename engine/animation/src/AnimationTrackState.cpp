/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#include "V8DataModel/AnimationTrackState.h"
#include "V8DataModel/Animation.h"
#include "V8DataModel/Animator.h"
#include "V8DataModel/KeyframeSequence.h"
#include "V8Tree/Service.h"
#include "Util/RunStateOwner.h"
#include "V8DataModel/Animation.h"
#include "V8datamodel/KeyframeSequenceProvider.h"

namespace RBX {

const char* const  sAnimationTrackState = "AnimationTrackState";
static const float autoStopFadeTime = 0.3f;

REFLECTION_BEGIN();
static Reflection::RemoteEventDesc<AnimationTrackState, void(float,float,float, float)> event_play(&AnimationTrackState::internalPlaySignal, "PlayAnimation", "gameTime", "fadeTime", "weight", "speed" , Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<AnimationTrackState, void(float,float)> event_stop(&AnimationTrackState::internalStopSignal, "StopAnimation", "gameTime", "fadeTime", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<AnimationTrackState, void(float,float,float)> event_adjustWeight(&AnimationTrackState::internalAdjustWeightSignal, "AdjustAnimationWeight", "gameTime", "weight", "fadeTime", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<AnimationTrackState, void(float,float)> event_adjustSpeed(&AnimationTrackState::internalAdjustSpeedSignal, "AdjustAnimation", "gameTime", "speed", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);

static Reflection::RemoteEventDesc<AnimationTrackState, void(std::string)> event_Keyframe(&AnimationTrackState::keyframeReachedSignal, "KeyframeReached", "keyframeName", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<AnimationTrackState, void()> event_Stopped(&AnimationTrackState::stoppedSignal, "Stopped", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
REFLECTION_END();

AnimationTrackState::AnimationTrackState(shared_ptr<const KeyframeSequence> keyframeSequence, weak_ptr<const Animator> animator)
: DescribedNonCreatable<AnimationTrackState, Instance, sAnimationTrackState>()
, startTime(0.0f)
, fadeStartTime(0.0f)
, fadeStartWeight(0.0f)
, fadeEndTime(0.0f)
, fadeEndWeight(0.0f)
, speed(1.0f)
, phase(0.0f)
, keyframeSequence(keyframeSequence)
, animator(animator)
, preKeyframe(-1)
, isPlaying(false)
, looped(keyframeSequence && keyframeSequence->getLoop())
, priority(KeyframeSequence::CORE)
, priorityOverridden(false)
{
	setName(sAnimationTrackState);

	internalPlaySignal.connect(boost::bind(&AnimationTrackState::onPlay, this, _1, _2, _3, _4));
	internalStopSignal.connect(boost::bind(&AnimationTrackState::onStop, this, _1, _2));
	internalAdjustWeightSignal.connect(boost::bind(&AnimationTrackState::onAdjustWeight, this, _1, _2, _3));
	internalAdjustSpeedSignal.connect(boost::bind(&AnimationTrackState::onAdjustSpeed, this, _1, _2));

	lockParent();
}

void AnimationTrackState::setAnimationTrack(shared_ptr<AnimationTrack> animationTrackIn)
{
	animationTrack = animationTrackIn;
}

bool AnimationTrackState::isStopped(double time)
{
	return (time >= fadeEndTime) && G3D::fuzzyEq(fadeEndWeight,0);
}
double AnimationTrackState::getGameTime()
{
	if(shared_ptr<const Animator> safeAnimator = animator.lock()){
		return safeAnimator->getGameTime();
	}
	//We've lost our animator, shut.it.down
	keyframeSequence.reset();
	return 0.0f;
}
double AnimationTrackState::getWeightAtTime(double time) const
{
	if(time >= fadeEndTime)
	{
		return fadeEndWeight;
	}
	else if(time <= fadeStartTime)
	{
		return fadeStartWeight;
	}
	else
	{
		// lerp between two.
		double interval = (fadeEndTime - fadeStartTime);
		return ((time - fadeStartTime)/ interval) * fadeEndWeight   +
				((fadeEndTime - time) / interval) * fadeStartWeight;
	}
}

double AnimationTrackState::getKeyframeAtTime(double time)
{
	if (!inReverse())
	{
		return (time - startTime) * speed + phase;
	}
	else
	{
		return (getDuration() - (time - startTime) * -speed) + phase;
	}
}

float AnimationTrackState::getDuration()
{
	if (const KeyframeSequence *pSequence = getKeyframeSequence())
	{
		return pSequence->getDuration();
	}

	return 0.0f;
}

KeyframeSequence::Priority AnimationTrackState::getPriority() const
{
	if (priorityOverridden) {
		return priority;
	}
	else
	{
		if (const KeyframeSequence* sequence = getKeyframeSequence())
		{
			return sequence->getPriority();
		}
		else
		{
			return KeyframeSequence::CORE;
		}
	}
}

void AnimationTrackState::setPriority(KeyframeSequence::Priority priorityIn)
{
	priorityOverridden = true;
	priority = priorityIn;
}

void AnimationTrackState::setKeyframeAtTime(double gameTime, double keyframetime)
{
	double delta = keyframetime - getKeyframeAtTime(gameTime);
	phase += delta;
}

void AnimationTrackState::onPlay(float gameTime, float fadeTime, float weight, float speed)
{
	if(gameTime >= startTime){
		phase = 0;
		startTime = gameTime;
		fadeStartTime = startTime;
		fadeStartWeight = 0.0;
		fadeEndTime = startTime + fadeTime;
		fadeEndWeight = weight;
		this->speed = speed;

		lastKeyframeTime = 0;
		preKeyframe = -1;

		isPlaying = true;

		//automated animation detection does not detect the first keyframe if the animation is running in reverse
		if (inReverse())
		{
			const KeyframeSequence* kfs = getKeyframeSequence();
			lastKeyframeTime = kfs ? kfs->getDuration() : 0.0;
			if (kfs->numChildren() > 0)
			{
				if (kfs->getChild(kfs->numChildren() - 1))
				{
					event_Keyframe.fireAndReplicateEvent(this, kfs->getChild(kfs->numChildren() - 1)->getName());
				}
			}
		}
	}
}

void AnimationTrackState::play(float fadeTime, float weight, float speed)
{
	event_play.fireAndReplicateEvent(this, (float)getGameTime(), fadeTime, weight, speed);
}

void AnimationTrackState::onStop(float gameTime, float fadeTime)
{
	const double currentWeight = getWeightAtTime(gameTime);
	fadeStartTime = gameTime;
	fadeStartWeight = currentWeight;
	fadeEndTime = gameTime + fadeTime;
	fadeEndWeight = 0.0;
	event_Stopped.fireAndReplicateEvent(this);

	isPlaying = false;
}

void AnimationTrackState::stop(float fadeTime)
{
	event_stop.fireAndReplicateEvent(this, (float)getGameTime(), fadeTime);
}

void AnimationTrackState::onAdjustWeight(float gameTime, float weight, float fadeTime)
{
	const double currentWeight = getWeightAtTime(gameTime);
	fadeStartTime = gameTime;
	fadeStartWeight = currentWeight;
	fadeEndTime = gameTime + fadeTime;
	fadeEndWeight = weight;
}

void AnimationTrackState::onAdjustSpeed(float gameTime, float speed)
{
	if(speed != this->speed)
	{
		//if we reversed directions, we need to reset the preKeyframe
		if ( (this->speed >= 0 && speed < 0) || (this->speed < 0 && speed >= 0) )
		{
			preKeyframe = -1;
		}

		// must adjust phase to prevent skipping.
		double keyframetime = getKeyframeAtTime(gameTime);
		this->speed = speed;
		setKeyframeAtTime(gameTime, keyframetime);
	}
}

void AnimationTrackState::adjustWeight(float weight, float fadeTime)
{
	event_adjustWeight.fireAndReplicateEvent(this, (float)getGameTime(), weight, fadeTime);
}

void AnimationTrackState::adjustSpeed(float speed)
{
	event_adjustSpeed.fireAndReplicateEvent(this, (float)getGameTime(), speed);
}

void AnimationTrackState::triggerKeyframeReachedSignal(const shared_ptr<Instance>& child, double minKeyframeTime, double maxKeyframeTime)
{
	Keyframe* kf = Instance::fastDynamicCast<Keyframe>(child.get());
	if(kf)
	{
		if(kf->getTime() > minKeyframeTime && kf->getTime() <= maxKeyframeTime)
		{
			event_Keyframe.fireAndReplicateEvent(this, kf->getName());	
		}
	}
}

bool AnimationTrackState::inReverse()
{
	return (speed < 0);
}

double AnimationTrackState::getDurationClampedKeyframeTime(double keyframeTime)
{
	const double duration = getDuration();
	if (duration > 0.0 && std::isfinite(keyframeTime))
	{
		if (keyframeTime < 0.0 || keyframeTime > duration)
		{
			keyframeTime = std::fmod(keyframeTime, duration);
			if (keyframeTime < 0.0)
				keyframeTime += duration;
			if (G3D::fuzzyEq(keyframeTime, 0.0))
				keyframeTime = duration;
		}
	}
	return keyframeTime;
}

void AnimationTrackState::resetKeyframeReachedDetection(double keyframeTime)
{
	lastKeyframeTime = keyframeTime;
	preKeyframe = -1;
}

void AnimationTrackState::step(std::vector<PoseAccumulator>& jointposes, double time)
{
	const KeyframeSequence* kfs = getKeyframeSequence();
	if (!kfs)
		return;
	double keyframetime = getKeyframeAtTime(time);
	float trackweight = (float)getWeightAtTime(time);
	float duration = kfs->getDuration();

	kfs->apply(jointposes, lastKeyframeTime, keyframetime, trackweight);
	detectKeyframeReached(keyframetime, lastKeyframeTime);

	if(!looped) // play one time. give stop command after trigger of last keyframe.
	{
		//determine if we are finished with the animation
		bool doneWithAnimation;
		if (!inReverse())
			doneWithAnimation = (lastKeyframeTime < duration && keyframetime >= duration);
		else
			doneWithAnimation = (lastKeyframeTime > 0 && keyframetime <= 0);
		if (duration <= 0)
			doneWithAnimation = true;

		if(doneWithAnimation)
		{
			onStop((float)time, autoStopFadeTime);
		}
	}
	
	lastKeyframeTime = keyframetime;
}

void AnimationTrackState::detectKeyframeReached(double animationTime, double lastAnimationTime)
{
	if (!isPlaying)
		return;

	const KeyframeSequence* kfs = getKeyframeSequence();
	if (!kfs || !std::isfinite(animationTime) ||
		!std::isfinite(lastAnimationTime) || animationTime == lastAnimationTime)
		return;

	struct KeyframeEntry
	{
		const Keyframe* keyframe;
		int sourceIndex;
	};
	std::vector<KeyframeEntry> keyframes;
	for (std::size_t index = 0; index < kfs->numChildren(); ++index)
	{
		if (const Keyframe* keyframe =
			Instance::fastDynamicCast<const Keyframe>(kfs->getChild(index)))
			keyframes.push_back({keyframe, static_cast<int>(index)});
	}
	if (keyframes.empty())
		return;
	std::stable_sort(keyframes.begin(), keyframes.end(),
		[](const KeyframeEntry& left, const KeyframeEntry& right)
		{
			return left.keyframe->getTime() < right.keyframe->getTime();
		});

	struct TimedEvent
	{
		double time;
		int order;
		const KeyframeEntry* keyframe;
		bool loop;
	};
	std::vector<TimedEvent> events;
	const bool reverse = animationTime < lastAnimationTime;
	const double duration = kfs->getDuration();
	constexpr std::size_t maxEventsPerStep = 4096;
	auto appendEvent = [&](double eventTime, int order,
		const KeyframeEntry* keyframe, bool loop)
	{
		if (events.size() < maxEventsPerStep)
			events.push_back({eventTime, order, keyframe, loop});
	};

	if (!looped || duration <= 0.0)
	{
		for (const KeyframeEntry& entry : keyframes)
		{
			const double eventTime = entry.keyframe->getTime();
			const bool crossed = reverse
				? eventTime < lastAnimationTime && eventTime >= animationTime
				: (eventTime > lastAnimationTime ||
					(preKeyframe < 0 && G3D::fuzzyEq(eventTime, lastAnimationTime))) &&
					eventTime <= animationTime;
			if (crossed)
				appendEvent(eventTime, 0, &entry, false);
		}
	}
	else
	{
		// Interior keyframes repeat once per cycle. Endpoints are emitted around
		// each loop boundary below so both authored boundary names remain visible.
		for (const KeyframeEntry& entry : keyframes)
		{
			const double keyTime = entry.keyframe->getTime();
			if (keyTime <= 0.0 || keyTime >= duration)
				continue;
			const double low = reverse ? animationTime : lastAnimationTime;
			const double high = reverse ? lastAnimationTime : animationTime;
			long long firstCycle = reverse
				? static_cast<long long>(std::ceil((low - keyTime) / duration))
				: static_cast<long long>(std::floor((low - keyTime) / duration)) + 1;
			long long lastCycle = reverse
				? static_cast<long long>(std::ceil((high - keyTime) / duration)) - 1
				: static_cast<long long>(std::floor((high - keyTime) / duration));
			for (long long cycle = firstCycle;
				cycle <= lastCycle && events.size() < maxEventsPerStep; ++cycle)
				appendEvent(keyTime + cycle * duration, 1, &entry, false);
		}

		const double low = reverse ? animationTime : lastAnimationTime;
		const double high = reverse ? lastAnimationTime : animationTime;
		long long firstBoundary = reverse
			? static_cast<long long>(std::ceil(low / duration))
			: static_cast<long long>(std::floor(low / duration)) + 1;
		long long lastBoundary = reverse
			? static_cast<long long>(std::ceil(high / duration)) - 1
			: static_cast<long long>(std::floor(high / duration));
		for (long long boundaryIndex = firstBoundary;
			boundaryIndex <= lastBoundary && events.size() < maxEventsPerStep;
			++boundaryIndex)
		{
			const double boundary = boundaryIndex * duration;
			if (reverse)
			{
				appendEvent(boundary, 0, &keyframes.front(), false);
				appendEvent(boundary, 1, NULL, true);
				appendEvent(boundary, 2, &keyframes.back(), false);
			}
			else
			{
				appendEvent(boundary, 0, &keyframes.back(), false);
				appendEvent(boundary, 1, NULL, true);
				appendEvent(boundary, 2, &keyframes.front(), false);
			}
		}
	}

	std::stable_sort(events.begin(), events.end(),
		[reverse](const TimedEvent& left, const TimedEvent& right)
		{
			if (left.time != right.time)
				return reverse ? left.time > right.time : left.time < right.time;
			return left.order < right.order;
		});
	for (const TimedEvent& event : events)
	{
		if (event.loop)
			didLoopSignal();
		else if (event.keyframe)
		{
			event_Keyframe.fireAndReplicateEvent(
				this, event.keyframe->keyframe->getName());
			preKeyframe = event.keyframe->sourceIndex;
		}
	}
}

}
