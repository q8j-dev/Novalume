
#include "V8DataModel/TweenService.h"
#include "V8DataModel/GuiObject.h"
#include "util/RunStateOwner.h"
#include "network/Players.h"

#include "reflection/enumconverter.h"

#include <algorithm>
#include <cmath>

namespace RBX
{
const char* const sTweenBase = "TweenBase";
const char* const sTween = "Tween";
const char* const sTweenService = "TweenService";

namespace Reflection {
template<> TweenInfo& Variant::convert<TweenInfo>()
{
    if (!isType<TweenInfo>())
        throw runtime_error("Unable to convert value to TweenInfo");
    return cast<TweenInfo>();
}

template<> const Type& Type::getSingleton<TweenInfo>()
{
    static TType<TweenInfo> type("TweenInfo");
    return type;
}

template<> EnumDesc<Enums::PlaybackState>::EnumDesc()
    : EnumDescriptor("PlaybackState")
{
    addPair(Enums::PLAYBACK_STATE_BEGIN, "Begin");
    addPair(Enums::PLAYBACK_STATE_DELAYED, "Delayed");
    addPair(Enums::PLAYBACK_STATE_PLAYING, "Playing");
    addPair(Enums::PLAYBACK_STATE_PAUSED, "Paused");
    addPair(Enums::PLAYBACK_STATE_COMPLETED, "Completed");
    addPair(Enums::PLAYBACK_STATE_CANCELLED, "Cancelled");
}
}

static Reflection::EnumPropDescriptor<TweenBase, Enums::PlaybackState>
    descPlaybackState("PlaybackState", category_Data,
        &TweenBase::getPlaybackState, NULL,
        Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::BoundFuncDesc<TweenBase, void()> funcPlay(
    &TweenBase::play, "Play", Security::None);
static Reflection::BoundFuncDesc<TweenBase, void()> funcPause(
    &TweenBase::pause, "Pause", Security::None);
static Reflection::BoundFuncDesc<TweenBase, void()> funcCancel(
    &TweenBase::cancel, "Cancel", Security::None);
static Reflection::EventDesc<TweenBase, void(Enums::PlaybackState)>
    eventCompleted(&TweenBase::completedSignal, "Completed", "playbackState");
static Reflection::RefPropDescriptor<Tween, Instance> descTweenInstance(
    "Instance", category_Data, &Tween::getTarget, NULL,
    Reflection::PropertyDescriptor::UI);
static Reflection::BoundFuncDesc<TweenService,
    shared_ptr<Instance>(shared_ptr<Instance>, TweenInfo,
        shared_ptr<const Reflection::ValueTable>)> funcCreateTween(
    &TweenService::createTween, "Create", "instance", "tweenInfo",
    "propertyTable", Security::None);

namespace {

float bounceOut(float value)
{
    if (value < 1.0f / 2.75f)
        return 7.5625f * value * value;
    if (value < 2.0f / 2.75f) {
        value -= 1.5f / 2.75f;
        return 7.5625f * value * value + 0.75f;
    }
    if (value < 2.5f / 2.75f) {
        value -= 2.25f / 2.75f;
        return 7.5625f * value * value + 0.9375f;
    }
    value -= 2.625f / 2.75f;
    return 7.5625f * value * value + 0.984375f;
}

float easeIn(GuiObject::TweenEasingStyle style, float value)
{
    constexpr float pi = 3.14159265358979323846f;
    switch (style) {
    case GuiObject::EASING_STYLE_LINEAR: return value;
    case GuiObject::EASING_STYLE_SINE: return 1.0f - std::cos(value * pi * 0.5f);
    case GuiObject::EASING_STYLE_BACK: {
        constexpr float c1 = 1.70158f;
        return (c1 + 1.0f) * value * value * value - c1 * value * value;
    }
    case GuiObject::EASING_STYLE_QUAD: return value * value;
    case GuiObject::EASING_STYLE_CUBIC: return value * value * value;
    case GuiObject::EASING_STYLE_QUART: return value * value * value * value;
    case GuiObject::EASING_STYLE_QUINT: return value * value * value * value * value;
    case GuiObject::EASING_STYLE_BOUNCE: return 1.0f - bounceOut(1.0f - value);
    case GuiObject::EASING_STYLE_ELASTIC:
        if (value <= 0.0f || value >= 1.0f) return value;
        return -std::pow(2.0f, 10.0f * value - 10.0f) *
            std::sin((value * 10.0f - 10.75f) * (2.0f * pi / 3.0f));
    case GuiObject::EASING_STYLE_EXPONENTIAL:
        return value <= 0.0f ? 0.0f : std::pow(2.0f, 10.0f * value - 10.0f);
    case GuiObject::EASING_STYLE_CIRCULAR:
        return 1.0f - std::sqrt(std::max(0.0f, 1.0f - value * value));
    default: return value;
    }
}

float ease(GuiObject::TweenEasingStyle style,
    GuiObject::TweenEasingDirection direction, float value)
{
    value = std::clamp(value, 0.0f, 1.0f);
    if (direction == GuiObject::EASING_DIRECTION_IN)
        return easeIn(style, value);
    if (direction == GuiObject::EASING_DIRECTION_OUT)
        return 1.0f - easeIn(style, 1.0f - value);
    return value < 0.5f
        ? easeIn(style, value * 2.0f) * 0.5f
        : 1.0f - easeIn(style, (1.0f - value) * 2.0f) * 0.5f;
}

Reflection::Variant normalizeGoal(const Reflection::Variant& start,
    const Reflection::Variant& goal)
{
    if (start.isType<float>()) return Reflection::Variant(goal.get<float>());
    if (start.isType<double>()) return Reflection::Variant(goal.get<double>());
    if (start.isType<int>()) return Reflection::Variant(goal.get<int>());
    if (start.isType<long long>()) return Reflection::Variant(goal.get<long long>());
    if (start.isType<UDim>()) return Reflection::Variant(goal.get<UDim>());
    if (start.isType<UDim2>()) return Reflection::Variant(goal.get<UDim2>());
    if (start.isType<Vector2>()) return Reflection::Variant(goal.get<Vector2>());
    if (start.isType<Vector3>()) return Reflection::Variant(goal.get<Vector3>());
    if (start.isType<Color3>()) return Reflection::Variant(goal.get<Color3>());
    if (start.isType<CoordinateFrame>())
        return Reflection::Variant(goal.get<CoordinateFrame>());
    if (start.isType<Rect2D>()) return Reflection::Variant(goal.get<Rect2D>());
    throw runtime_error("property type %s cannot be tweened", start.type().name.c_str());
}

Reflection::Variant interpolate(const Reflection::Variant& start,
    const Reflection::Variant& goal, float alpha)
{
    if (start.isType<float>())
        return start.cast<float>() + (goal.cast<float>() - start.cast<float>()) * alpha;
    if (start.isType<double>())
        return start.cast<double>() + (goal.cast<double>() - start.cast<double>()) * alpha;
    if (start.isType<int>())
        return static_cast<int>(std::lround(start.cast<int>() +
            (goal.cast<int>() - start.cast<int>()) * alpha));
    if (start.isType<long long>())
        return static_cast<long long>(std::llround(start.cast<long long>() +
            (goal.cast<long long>() - start.cast<long long>()) * alpha));
    if (start.isType<UDim>()) return start.cast<UDim>().lerp(goal.cast<UDim>(), alpha);
    if (start.isType<UDim2>()) return start.cast<UDim2>().lerp(goal.cast<UDim2>(), alpha);
    if (start.isType<Vector2>()) return start.cast<Vector2>().lerp(goal.cast<Vector2>(), alpha);
    if (start.isType<Vector3>()) return start.cast<Vector3>().lerp(goal.cast<Vector3>(), alpha);
    if (start.isType<Color3>()) return start.cast<Color3>().lerp(goal.cast<Color3>(), alpha);
    if (start.isType<CoordinateFrame>())
        return start.cast<CoordinateFrame>().lerp(goal.cast<CoordinateFrame>(), alpha);
    if (start.isType<Rect2D>()) return start.cast<Rect2D>().lerp(goal.cast<Rect2D>(), alpha);
    throw runtime_error("property type %s cannot be tweened", start.type().name.c_str());
}

} // namespace

TweenBase::TweenBase()
    : playbackState(Enums::PLAYBACK_STATE_BEGIN)
{
    setName(sTweenBase);
}

void TweenBase::setPlaybackState(Enums::PlaybackState value)
{
    if (playbackState == value) return;
    playbackState = value;
    raisePropertyChanged(descPlaybackState);
}

void TweenBase::play()
{
    if (playbackState == Enums::PLAYBACK_STATE_PLAYING ||
        playbackState == Enums::PLAYBACK_STATE_DELAYED)
        return;
    onPlay();
}

void TweenBase::pause()
{
    if (playbackState == Enums::PLAYBACK_STATE_PLAYING ||
        playbackState == Enums::PLAYBACK_STATE_DELAYED)
        setPlaybackState(Enums::PLAYBACK_STATE_PAUSED);
}

void TweenBase::cancel()
{
    if (playbackState == Enums::PLAYBACK_STATE_CANCELLED ||
        playbackState == Enums::PLAYBACK_STATE_COMPLETED)
        return;
    onCancel();
    setPlaybackState(Enums::PLAYBACK_STATE_CANCELLED);
    completedSignal(playbackState);
}

Tween::Tween(const shared_ptr<Instance>& instance, const TweenInfo& tweenInfo,
    const shared_ptr<const Reflection::ValueTable>& goals)
    : target(instance)
    , info(tweenInfo)
    , elapsed(0.0)
    , completedCycles(0)
{
    if (!instance) throw runtime_error("TweenService:Create requires an Instance");
    if (!goals || goals->empty())
        throw runtime_error("TweenService:Create requires a non-empty property table");
    if (!std::isfinite(info.time) || info.time < 0.0f ||
        !std::isfinite(info.delayTime) || info.delayTime < 0.0f || info.repeatCount < -1)
        throw runtime_error("TweenInfo contains invalid timing values");

    setName(sTween);
    properties.reserve(goals->size());
    for (const auto& goal : *goals) {
        Reflection::PropertyDescriptor* descriptor =
            instance->findPropertyDescriptor(goal.first.c_str());
        if (!descriptor || !descriptor->isScriptable())
            throw runtime_error("%s is not a valid tween property of %s",
                goal.first.c_str(), instance->getClassName().c_str());
        if (descriptor->isReadOnly())
            throw runtime_error("%s is read only", goal.first.c_str());
        Reflection::Variant start;
        descriptor->getVariant(instance.get(), start);
        properties.push_back(PropertyTarget{
            descriptor, start, normalizeGoal(start, goal.second)});
    }
}

void Tween::onPlay()
{
    if (playbackState != Enums::PLAYBACK_STATE_PAUSED) {
        elapsed = 0.0;
        completedCycles = 0;
    }
    setPlaybackState(elapsed < info.delayTime
        ? Enums::PLAYBACK_STATE_DELAYED
        : Enums::PLAYBACK_STATE_PLAYING);
}

void Tween::onCancel()
{
}

void Tween::apply(float alpha)
{
	shared_ptr<Instance> instance = target;
    if (!instance) return;
    const float eased = ease(info.easingStyle, info.easingDirection, alpha);
    for (const PropertyTarget& property : properties)
        property.descriptor->setVariant(instance.get(),
            interpolate(property.start, property.goal, eased));
}

bool Tween::step(double deltaTime)
{
    if (playbackState == Enums::PLAYBACK_STATE_COMPLETED ||
        playbackState == Enums::PLAYBACK_STATE_CANCELLED)
        return true;
    if (playbackState == Enums::PLAYBACK_STATE_BEGIN ||
        playbackState == Enums::PLAYBACK_STATE_PAUSED)
        return false;
	if (!target) {
        cancel();
        return true;
    }

    elapsed += std::max(0.0, deltaTime);
    if (elapsed < info.delayTime) {
        setPlaybackState(Enums::PLAYBACK_STATE_DELAYED);
        return false;
    }
    setPlaybackState(Enums::PLAYBACK_STATE_PLAYING);

    const double duration = std::max(0.0, static_cast<double>(info.time));
    const int segmentsPerCycle = info.reverses ? 2 : 1;
    const double activeTime = elapsed - info.delayTime;
    const bool instant = duration <= 1e-9;
    const long long segment = instant
        ? segmentsPerCycle
        : static_cast<long long>(activeTime / duration);
    const long long cycle = segment / segmentsPerCycle;
    const bool finite = info.repeatCount >= 0;
    const long long cycleCount = static_cast<long long>(info.repeatCount) + 1;
    if (finite && cycle >= cycleCount) {
        apply(info.reverses ? 0.0f : 1.0f);
        setPlaybackState(Enums::PLAYBACK_STATE_COMPLETED);
        completedSignal(playbackState);
        return true;
    }

    completedCycles = static_cast<int>(std::min<long long>(cycle, INT_MAX));
    float alpha = instant ? 1.0f
        : static_cast<float>(std::fmod(activeTime, duration) / duration);
    if (info.reverses && (segment % 2) != 0) alpha = 1.0f - alpha;
    apply(alpha);
    return false;
}

TweenService::TweenService() :
	IStepped(StepType_Render)
{
	setName(sTweenService);
}

shared_ptr<Instance> TweenService::createTween(shared_ptr<Instance> instance,
	TweenInfo tweenInfo,
	shared_ptr<const Reflection::ValueTable> propertyTable)
{
	shared_ptr<Tween> tween(new Tween(instance, tweenInfo, propertyTable));
	activeTweens.insert(tween);
	return tween;
}
void TweenService::addTweeningObject(boost::weak_ptr<GuiObject> guiObject)
{
	if(tweeningObjects.find(guiObject) == tweeningObjects.end())
	{
		tweeningObjects.insert(guiObject);
	}
}

void TweenService::addTweenCallback(boost::function<void(GuiObject::TweenStatus)> tweenCallbackFunc, GuiObject::TweenStatus tweenStatusForCallback)
{
	tweenCallbacks.push_back(std::pair<boost::function<void(GuiObject::TweenStatus)>, GuiObject::TweenStatus>(tweenCallbackFunc, tweenStatusForCallback) );
}

/*override*/ void TweenService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		tweenCallbacks.clear();
		activeTweens.clear();
	}

	Super::onServiceProvider(oldProvider, newProvider);
	
    if(newProvider && RBX::Network::Players::serverIsPresent(newProvider))
	{
		onServiceProviderHeartbeatInstance(oldProvider, newProvider);
	}
	else
	{
		onServiceProviderIStepped(oldProvider, newProvider);
	}
}

void TweenService::onStepped(const Stepped& event)
{
	update(event.gameStep);
}

void TweenService::onHeartbeat(const Heartbeat& heartbeatEvent)
{
	update(heartbeatEvent.wallStep);
}

void TweenService::update(const double step)
{	
	// call all the tween callbacks (for tweens that have finished/been cancelled)
	TweenCallbacks temp;
	tweenCallbacks.swap(temp);
	for(TweenCallbacks::iterator iter = temp.begin(); iter != temp.end(); iter++)
	{
		iter->first(iter->second);
	}

	// Step all the "tweening" objects
	for(TweeningObjectsList::iterator iter = tweeningObjects.begin(); iter != tweeningObjects.end();)
	{
		if(shared_ptr<GuiObject> guiObj = iter->lock())
		{
			if(guiObj->tweenStep(step))
			{
				//Return true when we are done with tweening
				tweeningObjects.erase(iter++);
			}
			else
			{
				++iter;
			}
		}
		else
		{
			tweeningObjects.erase(iter++);
		}
	}

	for (ActiveTweens::iterator iter = activeTweens.begin(); iter != activeTweens.end();)
	{
		if (shared_ptr<Tween> tween = iter->lock())
		{
			if (tween->step(step))
				activeTweens.erase(iter++);
			else
				++iter;
		}
		else
		{
			activeTweens.erase(iter++);
		}
	}
}

} // namespace RBX
