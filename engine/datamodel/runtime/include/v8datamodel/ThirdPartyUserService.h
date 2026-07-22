#pragma once

#include "v8datamodel/InputObject.h"
#include "v8datamodel/InteractionEnums.h"
#include "v8tree/Service.h"

#include "rbx/signal.h"

#include <string>

namespace RBX {

extern const char* const sThirdPartyUserService;

class ThirdPartyUserService
    : public DescribedNonCreatable<ThirdPartyUserService, Instance, sThirdPartyUserService>
    , public Service
{
public:
    ThirdPartyUserService();

    Enums::ChatRestrictionStatus getFriendCommunicationRestrictionStatus() const;
    bool getHasActiveUser() const;
    Enums::ChatRestrictionStatus getVoiceChatRestrictionStatusProperty() const;

    std::string getUserPlatformName();
    Enums::ChatRestrictionStatus getVoiceChatRestrictionStatus();
    bool haveActiveUser();
    bool isAccountSwitchingSupported();
    bool isChatRestrictionSupported();
    bool isSingleSignOnSupported();
    void showAccountPicker();
    void registerActiveUser(InputObject::UserInputType gamepadId,
        boost::function<void(int)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    rbx::signal<void(int)> activeUserSignedOutSignal;

private:
    void setHasActiveUser(bool value);

    Enums::ChatRestrictionStatus friendRestrictionStatus;
    Enums::ChatRestrictionStatus voiceRestrictionStatus;
    bool hasActiveUser;
    InputObject::UserInputType activeGamepad;
    std::string platformUserName;
};

} // namespace RBX
