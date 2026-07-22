#include "v8datamodel/ThirdPartyUserService.h"

#if defined(RBX_PLATFORM_DURANGO) || defined(RBX_PLATFORM_UWP)
#include "v8datamodel/PlatformService.h"
#endif

namespace RBX {

const char* const sThirdPartyUserService = "ThirdPartyUserService";

REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<ThirdPartyUserService, Enums::ChatRestrictionStatus>
    propFriendCommunicationRestrictionStatus(
        "FriendCommunicationRestrictionStatus", category_Data,
        &ThirdPartyUserService::getFriendCommunicationRestrictionStatus, NULL,
        Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
static Reflection::PropDescriptor<ThirdPartyUserService, bool> propHasActiveUser(
    "HasActiveUser", category_Data, &ThirdPartyUserService::getHasActiveUser, NULL,
    Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
static Reflection::EnumPropDescriptor<ThirdPartyUserService, Enums::ChatRestrictionStatus>
    propVoiceChatRestrictionStatus(
        "VoiceChatRestrictionStatus", category_Data,
        &ThirdPartyUserService::getVoiceChatRestrictionStatusProperty, NULL,
        Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);

static Reflection::BoundFuncDesc<ThirdPartyUserService, std::string()> funcGetUserPlatformName(
    &ThirdPartyUserService::getUserPlatformName, "GetUserPlatformName", Security::RobloxScript);
static Reflection::BoundFuncDesc<ThirdPartyUserService, Enums::ChatRestrictionStatus()>
    funcGetVoiceChatRestrictionStatus(&ThirdPartyUserService::getVoiceChatRestrictionStatus,
        "GetVoiceChatRestrictionStatus", Security::RobloxScript);
static Reflection::BoundFuncDesc<ThirdPartyUserService, bool()> funcHaveActiveUser(
    &ThirdPartyUserService::haveActiveUser, "HaveActiveUser", Security::RobloxScript);
static Reflection::BoundFuncDesc<ThirdPartyUserService, bool()> funcIsAccountSwitchingSupported(
    &ThirdPartyUserService::isAccountSwitchingSupported, "IsAccountSwitchingSupported",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<ThirdPartyUserService, bool()> funcIsChatRestrictionSupported(
    &ThirdPartyUserService::isChatRestrictionSupported, "IsChatRestrictionSupported",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<ThirdPartyUserService, bool()> funcIsSingleSignOnSupported(
    &ThirdPartyUserService::isSingleSignOnSupported, "IsSingleSignOnSupported",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<ThirdPartyUserService, void()> funcShowAccountPicker(
    &ThirdPartyUserService::showAccountPicker, "ShowAccountPicker", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<ThirdPartyUserService, int(InputObject::UserInputType)>
    funcRegisterActiveUser(&ThirdPartyUserService::registerActiveUser, "RegisterActiveUser",
        "gamepadId", Security::RobloxScript);
static Reflection::EventDesc<ThirdPartyUserService, void(int)> eventActiveUserSignedOut(
    &ThirdPartyUserService::activeUserSignedOutSignal, "ActiveUserSignedOut", "signOutStatus",
    Security::RobloxScript);
REFLECTION_END();

ThirdPartyUserService::ThirdPartyUserService()
    : Service(true)
    , friendRestrictionStatus(Enums::CHAT_RESTRICTION_NOT_RESTRICTED)
    , voiceRestrictionStatus(Enums::CHAT_RESTRICTION_NOT_RESTRICTED)
    , hasActiveUser(false)
    , activeGamepad(InputObject::TYPE_NONE)
{
    setName(sThirdPartyUserService);
    setRobloxLocked(true);
}

Enums::ChatRestrictionStatus ThirdPartyUserService::getFriendCommunicationRestrictionStatus() const
{
    return friendRestrictionStatus;
}

bool ThirdPartyUserService::getHasActiveUser() const
{
    return hasActiveUser;
}

Enums::ChatRestrictionStatus ThirdPartyUserService::getVoiceChatRestrictionStatusProperty() const
{
    return voiceRestrictionStatus;
}

std::string ThirdPartyUserService::getUserPlatformName()
{
    return platformUserName;
}

Enums::ChatRestrictionStatus ThirdPartyUserService::getVoiceChatRestrictionStatus()
{
    return voiceRestrictionStatus;
}

bool ThirdPartyUserService::haveActiveUser()
{
    return hasActiveUser;
}

bool ThirdPartyUserService::isAccountSwitchingSupported()
{
#if defined(RBX_PLATFORM_DURANGO) || defined(RBX_PLATFORM_UWP)
    return true;
#else
    return false;
#endif
}

bool ThirdPartyUserService::isChatRestrictionSupported()
{
#if defined(RBX_PLATFORM_DURANGO) || defined(RBX_PLATFORM_UWP)
    return true;
#else
    return false;
#endif
}

bool ThirdPartyUserService::isSingleSignOnSupported()
{
#if defined(RBX_PLATFORM_DURANGO) || defined(RBX_PLATFORM_UWP)
    return true;
#else
    return false;
#endif
}

void ThirdPartyUserService::showAccountPicker()
{
#if defined(RBX_PLATFORM_DURANGO) || defined(RBX_PLATFORM_UWP)
    if (!isAccountSwitchingSupported())
        return;

    if (ServiceProvider* provider = const_cast<ServiceProvider*>(
            ServiceProvider::findServiceProvider(this)))
    {
        if (PlatformService* platformService = ServiceProvider::find<PlatformService>(provider))
            platformService->popupAccountPickerUI(activeGamepad);
    }
#endif
}

void ThirdPartyUserService::registerActiveUser(InputObject::UserInputType gamepadId,
    boost::function<void(int)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!isSingleSignOnSupported())
    {
        resumeFunction(4); // AccountAuth_NoUserDetected in the platform contract.
        return;
    }

    activeGamepad = gamepadId;
    setHasActiveUser(true);
    resumeFunction(0); // AccountAuth_Success in the platform contract.
}

void ThirdPartyUserService::setHasActiveUser(bool value)
{
    if (hasActiveUser == value)
        return;
    hasActiveUser = value;
    raisePropertyChanged(propHasActiveUser);
}

} // namespace RBX
