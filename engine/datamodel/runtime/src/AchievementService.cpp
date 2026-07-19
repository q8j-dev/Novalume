#include "V8DataModel/AchievementService.h"

#include <stdexcept>

namespace RBX {

const char* const sAchievementService = "AchievementService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<AchievementService, bool()> funcIsAvailable(
    &AchievementService::isAvailable, "IsAvailable", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<AchievementService, bool(std::string)>
    funcHasAchieved(&AchievementService::hasAchieved, "HasAchieved",
        "achievementName", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<AchievementService, bool(std::string)>
    funcGrantAchievement(&AchievementService::grantAchievement, "GrantAchievement",
        "achievementName", Security::RobloxScript);
REFLECTION_END();

AchievementService::AchievementService()
    : Service(true)
{
    setName(sAchievementService);
    setRobloxLocked(true);
}

bool AchievementService::isAvailable()
{
    return false;
}

void AchievementService::hasAchieved(std::string,
    boost::function<void(bool)>, boost::function<void(std::string)> errorFunction)
{
    errorFunction(
        "Achievements are not available on this platform, hasAchieved() should not be called!");
}

void AchievementService::grantAchievement(std::string,
    boost::function<void(bool)>, boost::function<void(std::string)> errorFunction)
{
    errorFunction(
        "Achievements are not available on this platform, grantAchievement() should not be called!");
}

} // namespace RBX
