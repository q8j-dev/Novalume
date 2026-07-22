#include "v8datamodel/FeatureRestrictionManager.h"

#include <stdexcept>

namespace RBX {

const char* const sFeatureRestrictionManager = "FeatureRestrictionManager";

namespace Reflection {
template<> EnumDesc<Enums::FeatureRestrictionAbuseVector>::EnumDesc()
    : EnumDescriptor("FeatureRestrictionAbuseVector")
{
    addPair(Enums::FEATURE_RESTRICTION_EXPERIENCE_CHAT, "ExperienceChat");
    addPair(Enums::FEATURE_RESTRICTION_COMMUNICATION, "Communication");
}

template<> Enums::FeatureRestrictionAbuseVector&
Variant::convert<Enums::FeatureRestrictionAbuseVector>()
{
    return genericConvert<Enums::FeatureRestrictionAbuseVector>();
}
} // namespace Reflection

template<> bool StringConverter<Enums::FeatureRestrictionAbuseVector>::convertToValue(
    const std::string& text, Enums::FeatureRestrictionAbuseVector& value)
{
    return Reflection::EnumDesc<Enums::FeatureRestrictionAbuseVector>::singleton()
        .convertToValue(text.c_str(), value);
}

REFLECTION_BEGIN();
static Reflection::RemoteEventDesc<FeatureRestrictionManager, void(bool, long long)>
    eventTimeoutChatAttempt(&FeatureRestrictionManager::timeoutChatAttemptSignal,
        "TimeoutChatAttempt", "permanent", "endTime", Security::RobloxScript,
        Reflection::RemoteEventCommon::REPLICATE_ONLY,
        Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<FeatureRestrictionManager,
    void(bool, long long, long long, Enums::FeatureRestrictionAbuseVector)>
    eventFeatureTimeoutAttempt(&FeatureRestrictionManager::featureTimeoutAttemptSignal,
        "FeatureTimeoutAttempt", "permanent", "startTime", "endTime",
        "featureRestrictionAbuseVector", Security::RobloxScript,
        Reflection::RemoteEventCommon::REPLICATE_ONLY,
        Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<FeatureRestrictionManager,
    void(Enums::FeatureRestrictionAbuseVector)> eventFeatureTimeoutRestored(
        &FeatureRestrictionManager::featureTimeoutRestoredSignal,
        "FeatureTimeoutRestored", "featureRestrictionAbuseVector",
        Security::RobloxScript, Reflection::RemoteEventCommon::REPLICATE_ONLY,
        Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<FeatureRestrictionManager,
    void(Enums::FeatureRestrictionAbuseVector)> eventShowFeatureInterventionDetails(
        &FeatureRestrictionManager::showFeatureInterventionDetailsSignal,
        "ShowFeatureInterventionDetails", "featureRestrictionAbuseVector",
        Security::RobloxScript, Reflection::RemoteEventCommon::REPLICATE_ONLY,
        Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<FeatureRestrictionManager,
    void(Enums::FeatureRestrictionAbuseVector, bool)>
    eventShowFeatureInterventionDetailsV2(
        &FeatureRestrictionManager::showFeatureInterventionDetailsV2Signal,
        "ShowFeatureInterventionDetailsV2", "featureRestrictionAbuseVector",
        "isGameJoin", Security::RobloxScript,
        Reflection::RemoteEventCommon::REPLICATE_ONLY,
        Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<FeatureRestrictionManager, void()>
    eventRefreshFeatureRestrictions(
        &FeatureRestrictionManager::refreshFeatureRestrictionsSignal,
        "RefreshFeatureRestrictions", Security::RobloxScript,
        Reflection::RemoteEventCommon::SCRIPTING,
        Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<FeatureRestrictionManager,
    void(bool, bool, long long, long long,
        Enums::FeatureRestrictionAbuseVector)> eventUpdateClientFeatureTimeout(
        &FeatureRestrictionManager::updateClientFeatureTimeoutSignal,
        "UpdateClientFeatureTimeout", "active", "permanent", "startTime",
        "endTime", "featureRestrictionAbuseVector", Security::RobloxScript,
        Reflection::RemoteEventCommon::REPLICATE_ONLY,
        Reflection::RemoteEventCommon::BROADCAST);

REFLECTION_END();

FeatureRestrictionManager::FeatureRestrictionManager()
    : Service(true)
{
    setName(sFeatureRestrictionManager);
    setRobloxLocked(true);
    updateConnection = updateClientFeatureTimeoutSignal.connect(
        [this](bool active, bool permanent, long long startTime,
            long long endTime, Enums::FeatureRestrictionAbuseVector abuseVector) {
            updateClientFeatureTimeout(active, permanent, startTime, endTime,
                abuseVector);
        });
}

std::size_t FeatureRestrictionManager::restrictionIndex(
    Enums::FeatureRestrictionAbuseVector abuseVector)
{
    switch (abuseVector)
    {
    case Enums::FEATURE_RESTRICTION_EXPERIENCE_CHAT: return 0;
    case Enums::FEATURE_RESTRICTION_COMMUNICATION: return 1;
    default: throw std::invalid_argument("unknown feature-restriction abuse vector");
    }
}

FeatureRestrictionManager::Restriction FeatureRestrictionManager::getRestriction(
    Enums::FeatureRestrictionAbuseVector abuseVector) const
{
    return restrictions[restrictionIndex(abuseVector)];
}

void FeatureRestrictionManager::updateClientFeatureTimeout(bool active,
    bool permanent, long long startTime, long long endTime,
    Enums::FeatureRestrictionAbuseVector abuseVector)
{
    Restriction& restriction = restrictions[restrictionIndex(abuseVector)];
    const bool wasActive = restriction.active;
    restriction = {active, permanent, startTime, endTime};

    if (active)
        featureTimeoutAttemptSignal(permanent, startTime, endTime, abuseVector);
    else if (wasActive)
        featureTimeoutRestoredSignal(abuseVector);
}

void FeatureRestrictionManager::refreshFeatureRestrictions()
{
    eventRefreshFeatureRestrictions.fireAndReplicateEvent(this);
}

void FeatureRestrictionManager::showFeatureInterventionDetails(
    Enums::FeatureRestrictionAbuseVector abuseVector)
{
    eventShowFeatureInterventionDetails.fireAndReplicateEvent(this, abuseVector);
}

void FeatureRestrictionManager::showFeatureInterventionDetailsV2(
    Enums::FeatureRestrictionAbuseVector abuseVector, bool isGameJoin)
{
    eventShowFeatureInterventionDetailsV2.fireAndReplicateEvent(
        this, abuseVector, isGameJoin);
}

void FeatureRestrictionManager::timeoutChatAttempt(bool permanent,
    long long endTime)
{
    eventTimeoutChatAttempt.fireAndReplicateEvent(this, permanent, endTime);
}

} // namespace RBX
