#pragma once

#include "v8tree/Service.h"

#include <string>
#include <boost/function.hpp>

namespace RBX {

extern const char* const sAchievementService;

// Platform achievement protocol exposed to current CorePackages. Desktop and
// offline clients have no platform protocol, so availability is false and the
// two protocol operations reject if called, matching the supplied Player.
class AchievementService
    : public DescribedNonCreatable<AchievementService, Instance, sAchievementService>
    , public Service
{
public:
    AchievementService();

    bool isAvailable();
    void hasAchieved(std::string achievementName,
        boost::function<void(bool)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void grantAchievement(std::string achievementName,
        boost::function<void(bool)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
};

} // namespace RBX
