#pragma once

#include "V8Tree/Service.h"

#include "rbx/signal.h"

#include <array>

namespace RBX {

namespace Enums {
enum FeatureRestrictionAbuseVector
{
    FEATURE_RESTRICTION_EXPERIENCE_CHAT = 0,
    FEATURE_RESTRICTION_COMMUNICATION = 1
};
}

extern const char* const sFeatureRestrictionManager;

class FeatureRestrictionManager
    : public DescribedNonCreatable<FeatureRestrictionManager, Instance,
          sFeatureRestrictionManager>
    , public Service
{
public:
    FeatureRestrictionManager();

    struct Restriction
    {
        bool active = false;
        bool permanent = false;
        long long startTime = 0;
        long long endTime = 0;
    };

    Restriction getRestriction(Enums::FeatureRestrictionAbuseVector abuseVector) const;
    void updateClientFeatureTimeout(bool active, bool permanent,
        long long startTime, long long endTime,
        Enums::FeatureRestrictionAbuseVector abuseVector);
    void refreshFeatureRestrictions();
    void showFeatureInterventionDetails(
        Enums::FeatureRestrictionAbuseVector abuseVector);
    void showFeatureInterventionDetailsV2(
        Enums::FeatureRestrictionAbuseVector abuseVector, bool isGameJoin);
    void timeoutChatAttempt(bool permanent, long long endTime);

    rbx::remote_signal<void(bool, long long)> timeoutChatAttemptSignal;
    rbx::remote_signal<void(bool, long long, long long,
        Enums::FeatureRestrictionAbuseVector)> featureTimeoutAttemptSignal;
    rbx::remote_signal<void(Enums::FeatureRestrictionAbuseVector)>
        featureTimeoutRestoredSignal;
    rbx::remote_signal<void(Enums::FeatureRestrictionAbuseVector)>
        showFeatureInterventionDetailsSignal;
    rbx::remote_signal<void(Enums::FeatureRestrictionAbuseVector, bool)>
        showFeatureInterventionDetailsV2Signal;
    rbx::remote_signal<void()> refreshFeatureRestrictionsSignal;
    rbx::remote_signal<void(bool, bool, long long, long long,
        Enums::FeatureRestrictionAbuseVector)> updateClientFeatureTimeoutSignal;

private:
    static std::size_t restrictionIndex(
        Enums::FeatureRestrictionAbuseVector abuseVector);

    std::array<Restriction, 2> restrictions;
    rbx::signals::scoped_connection updateConnection;
};

} // namespace RBX
