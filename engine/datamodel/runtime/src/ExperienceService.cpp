#include "V8DataModel/ExperienceService.h"

#include "Script/ScriptContext.h"
#include "Util/standardout.h"

#include <stdexcept>

namespace RBX {

const char* const sExperienceService = "ExperienceService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<ExperienceService,
    void(std::string, shared_ptr<const Reflection::ValueTable>,
        Lua::WeakFunctionRef, Lua::WeakFunctionRef)> funcExecuteCrossExperienceCall(
            &ExperienceService::executeCrossExperienceCall,
            "ExecuteCrossExperienceCall", "callId", "params", "successCallback",
            "errorCallback", Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService, long long()>
    funcGetFollowUserId(&ExperienceService::getFollowUserId, "GetFollowUserId",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService,
    shared_ptr<const Reflection::ValueTable>()> funcGetPendingJoinAttempt(
        &ExperienceService::getPendingJoinAttempt, "GetPendingJoinAttempt",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService, std::string()>
    funcGetPlaceJoinState(&ExperienceService::getPlaceJoinState,
        "GetPlaceJoinState", Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService, int()>
    funcGetQueuePosition(&ExperienceService::getQueuePosition, "GetQueuePosition",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService,
    std::string(shared_ptr<const Reflection::ValueTable>)> funcLaunchExperience(
        &ExperienceService::launchExperience, "LaunchExperience", "params",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService,
    std::string(shared_ptr<const Reflection::ValueTable>, std::string)>
        funcLaunchExperienceFromSource(&ExperienceService::launchExperienceFromSource,
            "LaunchExperienceFromSource", "params", "source",
            Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService,
    void(shared_ptr<const Reflection::ValueTable>, std::string,
        Lua::WeakFunctionRef)> funcLaunchExperienceFromSourceWithCallback(
            &ExperienceService::launchExperienceFromSourceWithCallback,
            "LaunchExperienceFromSourceWithCallback", "params", "source",
            "callback", Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService,
    rbx::signals::connection(Lua::WeakFunctionRef)> funcRegisterForExperienceJoin(
        &ExperienceService::registerForExperienceJoin, "RegisterForExperienceJoin",
        "callback", Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService,
    rbx::signals::connection(Lua::WeakFunctionRef)> funcRegisterForExperienceLeave(
        &ExperienceService::registerForExperienceLeave, "RegisterForExperienceLeave",
        "callback", Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService,
    void(std::string, shared_ptr<const Reflection::ValueTable>)>
        funcStartCrossExperience(&ExperienceService::startCrossExperience,
            "StartCrossExperience", "type", "params", Security::RobloxScript);
static Reflection::BoundFuncDesc<ExperienceService,
    void(std::string, shared_ptr<const Reflection::ValueTable>)>
        funcStopCrossExperience(&ExperienceService::stopCrossExperience,
            "StopCrossExperience", "type", "params", Security::RobloxScript);

static Reflection::EventDesc<ExperienceService,
    void(shared_ptr<const Reflection::ValueTable>)> eventOnNewJoinAttempt(
        &ExperienceService::onNewJoinAttemptSignal, "OnNewJoinAttempt", "params",
        Security::RobloxScript);
static Reflection::EventDesc<ExperienceService,
    void(std::string, shared_ptr<const Reflection::ValueTable>)>
        eventOnCrossExperienceStarted(&ExperienceService::onCrossExperienceStartedSignal,
            "OnCrossExperienceStarted", "type", "params", Security::RobloxScript);
static Reflection::EventDesc<ExperienceService,
    void(std::string, shared_ptr<const Reflection::ValueTable>)>
        eventOnCrossExperienceStopped(&ExperienceService::onCrossExperienceStoppedSignal,
            "OnCrossExperienceStopped", "type", "params", Security::RobloxScript);
static Reflection::EventDesc<ExperienceService, void(std::string)>
    eventPlaceJoinStateChanged(&ExperienceService::placeJoinStateChangedSignal,
        "PlaceJoinStateChanged", "state", Security::RobloxScript);
static Reflection::EventDesc<ExperienceService, void(int)>
    eventQueuePositionChanged(&ExperienceService::queuePositionChangedSignal,
        "QueuePositionChanged", "position", Security::RobloxScript);
REFLECTION_END();

ExperienceService::ExperienceService()
    : Service(true)
    , pendingJoinAttempt(emptyTable())
    , queuePosition(0)
    , followUserId(0)
{
    setName(sExperienceService);
    setRobloxLocked(true);
}

shared_ptr<const Reflection::ValueTable> ExperienceService::emptyTable()
{
    return shared_ptr<const Reflection::ValueTable>(new Reflection::ValueTable());
}

void ExperienceService::invoke(Lua::WeakFunctionRef callback,
    const Reflection::Tuple& arguments)
{
    if (!callback.lock())
        return;
    try
    {
        ServiceProvider::create<ScriptContext>(this)->callInNewThread(callback, arguments);
    }
    catch (const base_exception& error)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR,
            "ExperienceService callback failed: %s", error.what());
    }
}

void ExperienceService::executeCrossExperienceCall(std::string callId,
    shared_ptr<const Reflection::ValueTable> params,
    Lua::WeakFunctionRef successCallback, Lua::WeakFunctionRef errorCallback)
{
    Reflection::Tuple arguments;
    std::map<std::string, shared_ptr<const Reflection::ValueTable> >::const_iterator active =
        crossExperiences.find(callId);
    if (active != crossExperiences.end())
    {
        arguments.values.push_back(Reflection::Variant(
            params ? params : active->second));
        invoke(successCallback, arguments);
        return;
    }

    arguments.values.push_back(Reflection::Variant(std::string(
        "No active cross-experience endpoint for call '" + callId + "'")));
    invoke(errorCallback, arguments);
}

long long ExperienceService::getFollowUserId() { return followUserId; }
shared_ptr<const Reflection::ValueTable> ExperienceService::getPendingJoinAttempt()
{ return pendingJoinAttempt; }
std::string ExperienceService::getPlaceJoinState() { return placeJoinState; }
int ExperienceService::getQueuePosition() { return queuePosition; }

std::string ExperienceService::launchExperience(
    shared_ptr<const Reflection::ValueTable> params)
{
    return launchExperienceFromSource(params, std::string());
}

std::string ExperienceService::launchExperienceFromSource(
    shared_ptr<const Reflection::ValueTable> params, std::string source)
{
    shared_ptr<Reflection::ValueTable> attempt(new Reflection::ValueTable(
        params ? *params : Reflection::ValueTable()));
    if (!source.empty())
        (*attempt)["source"] = Reflection::Variant(source);
    pendingJoinAttempt = attempt;
    setPlaceJoinState("RequestingGame");
    onNewJoinAttemptSignal(pendingJoinAttempt);
    return std::string();
}

void ExperienceService::launchExperienceFromSourceWithCallback(
    shared_ptr<const Reflection::ValueTable> params, std::string source,
    Lua::WeakFunctionRef callback)
{
    const std::string result = launchExperienceFromSource(params, std::move(source));
    Reflection::Tuple arguments;
    arguments.values.push_back(Reflection::Variant(result));
    invoke(callback, arguments);
}

rbx::signals::connection ExperienceService::registerForExperienceJoin(
    Lua::WeakFunctionRef callback)
{
    return experienceJoinSignal.connect([this, callback]() mutable {
        invoke(callback, Reflection::Tuple());
    });
}

rbx::signals::connection ExperienceService::registerForExperienceLeave(
    Lua::WeakFunctionRef callback)
{
    return experienceLeaveSignal.connect([this, callback]() mutable {
        invoke(callback, Reflection::Tuple());
    });
}

void ExperienceService::startCrossExperience(std::string type,
    shared_ptr<const Reflection::ValueTable> params)
{
    if (!params)
        params = emptyTable();
    crossExperiences[type] = params;
    onCrossExperienceStartedSignal(type, params);
}

void ExperienceService::stopCrossExperience(std::string type,
    shared_ptr<const Reflection::ValueTable> params)
{
    if (!params)
        params = emptyTable();
    crossExperiences.erase(type);
    onCrossExperienceStoppedSignal(type, params);
}

void ExperienceService::setPlaceJoinState(std::string state)
{
    if (placeJoinState == state)
        return;
    placeJoinState = std::move(state);
    placeJoinStateChangedSignal(placeJoinState);
}

void ExperienceService::setQueuePosition(int position)
{
    if (queuePosition == position)
        return;
    queuePosition = position;
    queuePositionChangedSignal(queuePosition);
}

void ExperienceService::setFollowUserId(long long userId) { followUserId = userId; }
void ExperienceService::notifyExperienceJoined()
{
    setPlaceJoinState("Joined");
    experienceJoinSignal();
}
void ExperienceService::notifyExperienceLeaving()
{
    setPlaceJoinState("Leaving");
    experienceLeaveSignal();
}

} // namespace RBX
