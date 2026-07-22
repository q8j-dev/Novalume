/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "v8tree/Instance.h"
#include "v8tree/Service.h"
#include "util/HeartbeatInstance.h"
#include "util/SteppedInstance.h"
#include "v8datamodel/GuiObject.h"

namespace RBX
{
	namespace Enums
	{
		enum PlaybackState
		{
			PLAYBACK_STATE_BEGIN = 0,
			PLAYBACK_STATE_DELAYED = 1,
			PLAYBACK_STATE_PLAYING = 2,
			PLAYBACK_STATE_PAUSED = 3,
			PLAYBACK_STATE_COMPLETED = 4,
			PLAYBACK_STATE_CANCELLED = 5
		};
	}

	struct TweenInfo
	{
		float time;
		GuiObject::TweenEasingStyle easingStyle;
		GuiObject::TweenEasingDirection easingDirection;
		int repeatCount;
		bool reverses;
		float delayTime;

		TweenInfo(float time = 1.0f,
			GuiObject::TweenEasingStyle easingStyle = GuiObject::EASING_STYLE_QUAD,
			GuiObject::TweenEasingDirection easingDirection = GuiObject::EASING_DIRECTION_OUT,
			int repeatCount = 0, bool reverses = false, float delayTime = 0.0f)
			: time(time), easingStyle(easingStyle), easingDirection(easingDirection),
			  repeatCount(repeatCount), reverses(reverses), delayTime(delayTime) {}

		bool operator==(const TweenInfo& other) const
		{
			return time == other.time && easingStyle == other.easingStyle &&
				easingDirection == other.easingDirection &&
				repeatCount == other.repeatCount && reverses == other.reverses &&
				delayTime == other.delayTime;
		}
	};

	extern const char* const sTweenBase;
	extern const char* const sTween;
	extern const char* const sTweenService;

	class TweenBase
		: public DescribedNonCreatable<TweenBase, Instance, sTweenBase,
			Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
	private:
		typedef DescribedNonCreatable<TweenBase, Instance, sTweenBase,
			Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;
	public:
		TweenBase();

		Enums::PlaybackState getPlaybackState() const { return playbackState; }
		void play();
		void pause();
		void cancel();

		rbx::signal<void(Enums::PlaybackState)> completedSignal;

	protected:
		virtual void onPlay() = 0;
		virtual void onCancel() = 0;
		void setPlaybackState(Enums::PlaybackState value);
		Enums::PlaybackState playbackState;
	};

	class Tween
		: public DescribedNonCreatable<Tween, TweenBase, sTween,
			Reflection::ClassDescriptor::RUNTIME_LOCAL>
	{
	private:
		typedef DescribedNonCreatable<Tween, TweenBase, sTween,
			Reflection::ClassDescriptor::RUNTIME_LOCAL> Super;
		struct PropertyTarget
		{
			const Reflection::PropertyDescriptor* descriptor;
			Reflection::Variant start;
			Reflection::Variant goal;
		};
	public:
		Tween(const shared_ptr<Instance>& target, const TweenInfo& info,
			const shared_ptr<const Reflection::ValueTable>& goals);

		Instance* getTarget() const { return target.get(); }
		bool step(double deltaTime);

	protected:
		void onPlay() override;
		void onCancel() override;

	private:
		void apply(float alpha);
		shared_ptr<Instance> target;
		TweenInfo info;
		std::vector<PropertyTarget> properties;
		double elapsed;
		int completedCycles;
	};

	class TweenService
		: public DescribedCreatable<TweenService, Instance, sTweenService,
			Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
		, public HeartbeatInstance
		, public IStepped
	{
	private:
		typedef DescribedCreatable<TweenService, Instance, sTweenService,
			Reflection::ClassDescriptor::INTERNAL_LOCAL> Super;
	public:
		TweenService();
		shared_ptr<Instance> createTween(shared_ptr<Instance> instance,
			TweenInfo tweenInfo,
			shared_ptr<const Reflection::ValueTable> propertyTable);

		void addTweeningObject(boost::weak_ptr<GuiObject> guiObject);

		void addTweenCallback(boost::function<void(GuiObject::TweenStatus)> tweenCallbackFunc, GuiObject::TweenStatus tweenStatusForCallback);

	protected:
		typedef std::set<boost::weak_ptr<GuiObject> > TweeningObjectsList;
		
		TweeningObjectsList tweeningObjects;
		typedef std::set<boost::weak_ptr<Tween> > ActiveTweens;
		ActiveTweens activeTweens;

		/*override*/ void onHeartbeat(const Heartbeat& heartbeatEvent);
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	private:
		typedef std::vector<std::pair<boost::function<void(GuiObject::TweenStatus)>, GuiObject::TweenStatus> > TweenCallbacks;

		TweenCallbacks tweenCallbacks;

		void update(const double step);

		// IStepped
		virtual void onStepped(const Stepped& event);
	};
}
