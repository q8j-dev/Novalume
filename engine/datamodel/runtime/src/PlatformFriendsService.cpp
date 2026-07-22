#include "v8datamodel/PlatformFriendsService.h"

namespace RBX {

const char* const sPlatformFriendsService = "PlatformFriendsService";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<PlatformFriendsService, bool()>
    funcIsInviteFriendsEnabled(
        &PlatformFriendsService::isInviteFriendsEnabled,
        "IsInviteFriendsEnabled", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformFriendsService, bool()>
    funcIsProfileEnabled(&PlatformFriendsService::isProfileEnabled,
        "IsProfileEnabled", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformFriendsService, void()>
    funcShowInviteFriendsUI(&PlatformFriendsService::showInviteFriendsUI,
        "ShowInviteFriendsUI", Security::RobloxScript);
static Reflection::BoundFuncDesc<PlatformFriendsService, void(std::string)>
    funcShowProfile(&PlatformFriendsService::showProfile, "ShowProfile",
        "platformUserId", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PlatformFriendsService,
    shared_ptr<const Reflection::ValueArray>()> funcGetPartyMembers(
        &PlatformFriendsService::getPartyMembers, "GetPartyMembers",
        Security::RobloxScript);
REFLECTION_END();

PlatformFriendsService::PlatformFriendsService()
    : Service(true)
    , inviteFriendsEnabled(false)
    , profileEnabled(false)
    , partyMembers(new Reflection::ValueArray())
{
    setName(sPlatformFriendsService);
    setRobloxLocked(true);
}

void PlatformFriendsService::showInviteFriendsUI()
{
    if (inviteFriendsEnabled)
        showInviteFriendsUIRequested();
}

void PlatformFriendsService::showProfile(std::string platformUserId)
{
    if (profileEnabled && !platformUserId.empty())
        showProfileRequested(platformUserId);
}

void PlatformFriendsService::getPartyMembers(
    boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,
    boost::function<void(std::string)>)
{
    resumeFunction(partyMembers);
}

void PlatformFriendsService::setPartyMembers(
    shared_ptr<const Reflection::ValueArray> value)
{
    partyMembers = value ? value :
        shared_ptr<const Reflection::ValueArray>(new Reflection::ValueArray());
}

} // namespace RBX
