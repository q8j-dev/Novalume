#include "v8datamodel/GenericChallengeService.h"

namespace RBX {

const char* const sGenericChallengeService = "GenericChallengeService";

REFLECTION_BEGIN();
static Reflection::RemoteEventDesc<GenericChallengeService, void(std::string)>
    eventChallengeAbandoned(&GenericChallengeService::challengeAbandonedSignal,
        "ChallengeAbandonedEvent", "challengeID", Security::RobloxScript,
        Reflection::RemoteEventCommon::SCRIPTING,
        Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GenericChallengeService,
    void(std::string, std::string, std::string)> eventChallengeCompleted(
        &GenericChallengeService::challengeCompletedSignal,
        "ChallengeCompletedEvent", "challengeID", "challengeType",
        "challengeMetadata", Security::RobloxScript,
        Reflection::RemoteEventCommon::SCRIPTING,
        Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GenericChallengeService, void(std::string)>
    eventChallengeInvalidated(&GenericChallengeService::challengeInvalidatedSignal,
        "ChallengeInvalidatedEvent", "challengeID", Security::RobloxScript,
        Reflection::RemoteEventCommon::SCRIPTING,
        Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GenericChallengeService,
    void(std::string, bool)> eventChallengeLoaded(
        &GenericChallengeService::challengeLoadedSignal, "ChallengeLoadedEvent",
        "challengeID", "success", Security::RobloxScript,
        Reflection::RemoteEventCommon::SCRIPTING,
        Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GenericChallengeService,
    void(std::string, std::string, std::string)> eventChallengeRequired(
        &GenericChallengeService::challengeRequiredSignal,
        "ChallengeRequiredEvent", "challengeID", "challengeType",
        "challengeMetadata", Security::RobloxScript,
        Reflection::RemoteEventCommon::SCRIPTING,
        Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::BoundFuncDesc<GenericChallengeService, void(std::string)>
    funcSignalChallengeAbandoned(
        &GenericChallengeService::signalChallengeAbandoned,
        "SignalChallengeAbandoned", "challengeID", Security::RobloxScript);
static Reflection::BoundFuncDesc<GenericChallengeService,
    void(std::string, std::string, std::string)> funcSignalChallengeCompleted(
        &GenericChallengeService::signalChallengeCompleted,
        "SignalChallengeCompleted", "challengeID", "challengeType",
        "challengeMetadata", Security::RobloxScript);
static Reflection::BoundFuncDesc<GenericChallengeService, void(std::string)>
    funcSignalChallengeInvalidated(
        &GenericChallengeService::signalChallengeInvalidated,
        "SignalChallengeInvalidated", "challengeID", Security::RobloxScript);
static Reflection::BoundFuncDesc<GenericChallengeService,
    void(std::string, bool)> funcSignalChallengeLoaded(
        &GenericChallengeService::signalChallengeLoaded,
        "SignalChallengeLoaded", "challengeID", "success",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<GenericChallengeService,
    void(std::string, std::string, std::string)> funcSignalChallengeRequired(
        &GenericChallengeService::signalChallengeRequired,
        "SignalChallengeRequired", "challengeID", "challengeType",
        "challengeMetadata", Security::RobloxScript);
REFLECTION_END();

GenericChallengeService::GenericChallengeService()
    : Service(true)
{
    setName(sGenericChallengeService);
    setRobloxLocked(true);
}

void GenericChallengeService::signalChallengeAbandoned(std::string challengeId)
{
    eventChallengeAbandoned.fireAndReplicateEvent(this, challengeId);
}

void GenericChallengeService::signalChallengeCompleted(std::string challengeId,
    std::string challengeType, std::string challengeMetadata)
{
    eventChallengeCompleted.fireAndReplicateEvent(this, challengeId,
        challengeType, challengeMetadata);
}

void GenericChallengeService::signalChallengeInvalidated(std::string challengeId)
{
    eventChallengeInvalidated.fireAndReplicateEvent(this, challengeId);
}

void GenericChallengeService::signalChallengeLoaded(std::string challengeId,
    bool success)
{
    eventChallengeLoaded.fireAndReplicateEvent(this, challengeId, success);
}

void GenericChallengeService::signalChallengeRequired(std::string challengeId,
    std::string challengeType, std::string challengeMetadata)
{
    eventChallengeRequired.fireAndReplicateEvent(this, challengeId,
        challengeType, challengeMetadata);
}

} // namespace RBX
