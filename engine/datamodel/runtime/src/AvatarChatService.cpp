#include "V8DataModel/AvatarChatService.h"

namespace RBX {

const char* const sAvatarChatService = "AvatarChatService";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<AvatarChatService, int> propClientFeatures(
    "ClientFeatures", category_State, &AvatarChatService::getClientFeatures, NULL,
    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<AvatarChatService, bool> propClientFeaturesInitialized(
    "ClientFeaturesInitialized", category_State,
    &AvatarChatService::getClientFeaturesInitialized, NULL,
    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<AvatarChatService, int> propServerFeatures(
    "ServerFeatures", category_State, &AvatarChatService::getServerFeatures,
    &AvatarChatService::setServerFeatures, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING,
    Security::Roblox);

static Reflection::BoundFuncDesc<AvatarChatService, long long(std::string, long long)>
    funcDebugCounterGet(&AvatarChatService::debugCounterGet, "DebugCounterGet", "label",
        "playerId", Security::RobloxScript);
static Reflection::BoundFuncDesc<AvatarChatService, bool()> funcEnableVoice(
    &AvatarChatService::enableVoice, "EnableVoice", Security::RobloxScript);
static Reflection::BoundFuncDesc<AvatarChatService,
    bool(int, Enums::AvatarChatServiceFeature)> funcIsEnabled(
        &AvatarChatService::isEnabled, "IsEnabled", "mask", "feature",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<AvatarChatService, bool()> funcIsPlaceEnabled(
    &AvatarChatService::isPlaceEnabled, "IsPlaceEnabled", Security::RobloxScript);
static Reflection::BoundFuncDesc<AvatarChatService, bool()> funcIsUniverseEnabled(
    &AvatarChatService::isUniverseEnabled, "IsUniverseEnabled", Security::RobloxScript);
static Reflection::BoundFuncDesc<AvatarChatService, int()> funcPollClientFeatures(
    &AvatarChatService::pollClientFeatures, "PollClientFeatures", Security::RobloxScript);
static Reflection::BoundFuncDesc<AvatarChatService, int()> funcPollServerFeatures(
    &AvatarChatService::pollServerFeatures, "PollServerFeatures", Security::RobloxScript);
static Reflection::BoundFuncDesc<AvatarChatService, bool(Enums::DeviceFeatureType)>
    funcDeviceMeetsRequirements(&AvatarChatService::deviceMeetsRequirementsForFeature,
        "deviceMeetsRequirementsForFeature", "feature", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<AvatarChatService, int()> funcGetClientFeaturesAsync(
    &AvatarChatService::getClientFeaturesAsync, "GetClientFeaturesAsync",
    Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<AvatarChatService, int()> funcGetServerFeaturesAsync(
    &AvatarChatService::getServerFeaturesAsync, "GetServerFeaturesAsync",
    Security::RobloxScript);
REFLECTION_END();

AvatarChatService::AvatarChatService()
    : Service(true)
    , clientFeatures(0)
    , serverFeatures(0)
    , deviceFeatures((1 << Enums::DEVICE_FEATURE_CAPTURE) |
          (1 << Enums::DEVICE_FEATURE_IN_EXPERIENCE_FAE))
    , clientFeaturesInitialized(false)
{
    setName(sAvatarChatService);
    setRobloxLocked(true);
}

void AvatarChatService::setClientFeatures(int value)
{
    const bool initializedChanged = !clientFeaturesInitialized;
    if (clientFeatures != value)
    {
        clientFeatures = value;
        raisePropertyChanged(propClientFeatures);
    }
    clientFeaturesInitialized = true;
    if (initializedChanged)
        raisePropertyChanged(propClientFeaturesInitialized);
}

void AvatarChatService::setServerFeatures(int value)
{
    if (serverFeatures == value)
        return;
    serverFeatures = value;
    raisePropertyChanged(propServerFeatures);
}

long long AvatarChatService::debugCounterGet(std::string label, long long playerId)
{
    return debugCounters[std::make_pair(label, playerId)];
}

bool AvatarChatService::enableVoice()
{
    if (!isEnabled(clientFeatures, Enums::AVATAR_CHAT_USER_AUDIO_ELIGIBLE) ||
        isEnabled(clientFeatures, Enums::AVATAR_CHAT_USER_BANNED))
        return false;
    enableVoiceRequested();
    setClientFeatures(clientFeatures | Enums::AVATAR_CHAT_USER_AUDIO);
    return true;
}

bool AvatarChatService::isEnabled(int mask,
    Enums::AvatarChatServiceFeature feature)
{
    return feature != Enums::AVATAR_CHAT_NONE && (mask & static_cast<int>(feature)) != 0;
}

bool AvatarChatService::isPlaceEnabled()
{
    return (serverFeatures & (Enums::AVATAR_CHAT_PLACE_AUDIO |
        Enums::AVATAR_CHAT_PLACE_VIDEO)) != 0;
}

bool AvatarChatService::isUniverseEnabled()
{
    return (serverFeatures & (Enums::AVATAR_CHAT_UNIVERSE_AUDIO |
        Enums::AVATAR_CHAT_UNIVERSE_VIDEO)) != 0;
}

bool AvatarChatService::deviceMeetsRequirementsForFeature(
    Enums::DeviceFeatureType feature)
{
    return (deviceFeatures & (1 << static_cast<int>(feature))) != 0;
}

void AvatarChatService::setDeviceFeatureAvailable(Enums::DeviceFeatureType feature,
    bool available)
{
    const int bit = 1 << static_cast<int>(feature);
    if (available) deviceFeatures |= bit; else deviceFeatures &= ~bit;
}

void AvatarChatService::getClientFeaturesAsync(
    boost::function<void(int)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!clientFeaturesInitialized)
        setClientFeatures(clientFeatures);
    resumeFunction(clientFeatures);
}

void AvatarChatService::getServerFeaturesAsync(
    boost::function<void(int)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    resumeFunction(serverFeatures);
}

} // namespace RBX
