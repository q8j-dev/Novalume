#pragma once

#include "V8Tree/Service.h"

#include "rbx/signal.h"

namespace RBX {

extern const char* const sPlatformFriendsService;

class PlatformFriendsService
    : public DescribedNonCreatable<PlatformFriendsService, Instance,
          sPlatformFriendsService>
    , public Service
{
public:
    PlatformFriendsService();

    bool isInviteFriendsEnabled() { return inviteFriendsEnabled; }
    bool isProfileEnabled() { return profileEnabled; }
    void showInviteFriendsUI();
    void showProfile(std::string platformUserId);
    void getPartyMembers(
        boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    void setInviteFriendsEnabled(bool value) { inviteFriendsEnabled = value; }
    void setProfileEnabled(bool value) { profileEnabled = value; }
    void setPartyMembers(shared_ptr<const Reflection::ValueArray> value);

    rbx::signal<void()> showInviteFriendsUIRequested;
    rbx::signal<void(std::string)> showProfileRequested;

private:
    bool inviteFriendsEnabled;
    bool profileEnabled;
    shared_ptr<const Reflection::ValueArray> partyMembers;
};

} // namespace RBX
