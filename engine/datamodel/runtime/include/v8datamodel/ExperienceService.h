#pragma once

#include "Script/ThreadRef.h"
#include "V8Tree/Service.h"

#include "rbx/signal.h"

#include <map>
#include <string>

namespace RBX {

extern const char* const sExperienceService;

class ExperienceService
    : public DescribedNonCreatable<ExperienceService, Instance, sExperienceService>
    , public Service
{
public:
    ExperienceService();

    void executeCrossExperienceCall(std::string callId,
        shared_ptr<const Reflection::ValueTable> params,
        Lua::WeakFunctionRef successCallback, Lua::WeakFunctionRef errorCallback);
    long long getFollowUserId();
    shared_ptr<const Reflection::ValueTable> getPendingJoinAttempt();
    std::string getPlaceJoinState();
    int getQueuePosition();

    std::string launchExperience(shared_ptr<const Reflection::ValueTable> params);
    std::string launchExperienceFromSource(
        shared_ptr<const Reflection::ValueTable> params, std::string source);
    void launchExperienceFromSourceWithCallback(
        shared_ptr<const Reflection::ValueTable> params, std::string source,
        Lua::WeakFunctionRef callback);

    rbx::signals::connection registerForExperienceJoin(Lua::WeakFunctionRef callback);
    rbx::signals::connection registerForExperienceLeave(Lua::WeakFunctionRef callback);

    void startCrossExperience(std::string type,
        shared_ptr<const Reflection::ValueTable> params);
    void stopCrossExperience(std::string type,
        shared_ptr<const Reflection::ValueTable> params);

    void setPlaceJoinState(std::string state);
    void setQueuePosition(int position);
    void setFollowUserId(long long userId);
    void notifyExperienceJoined();
    void notifyExperienceLeaving();

    rbx::signal<void(shared_ptr<const Reflection::ValueTable>)> onNewJoinAttemptSignal;
    rbx::signal<void(std::string, shared_ptr<const Reflection::ValueTable>)>
        onCrossExperienceStartedSignal;
    rbx::signal<void(std::string, shared_ptr<const Reflection::ValueTable>)>
        onCrossExperienceStoppedSignal;
    rbx::signal<void(std::string)> placeJoinStateChangedSignal;
    rbx::signal<void(int)> queuePositionChangedSignal;

private:
    void invoke(Lua::WeakFunctionRef callback, const Reflection::Tuple& arguments);
    static shared_ptr<const Reflection::ValueTable> emptyTable();

    shared_ptr<const Reflection::ValueTable> pendingJoinAttempt;
    std::map<std::string, shared_ptr<const Reflection::ValueTable> > crossExperiences;
    std::string placeJoinState;
    int queuePosition;
    long long followUserId;
    rbx::signal<void()> experienceJoinSignal;
    rbx::signal<void()> experienceLeaveSignal;
};

} // namespace RBX
