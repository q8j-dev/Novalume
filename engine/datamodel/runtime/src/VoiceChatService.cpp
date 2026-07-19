#include "V8DataModel/VoiceChatService.h"

#include "Util/Guid.h"

namespace RBX {

const char* const sVoiceChatService = "VoiceChatService";

REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<VoiceChatService, Enums::VoiceChatDistanceAttenuationType>
    propDefaultDistanceAttenuation("DefaultDistanceAttenuation", category_Behavior,
        &VoiceChatService::getDefaultDistanceAttenuation,
        &VoiceChatService::setDefaultDistanceAttenuation,
        Reflection::PropertyDescriptor::STANDARD, Security::Plugin);
static Reflection::PropDescriptor<VoiceChatService, bool> propEnableDefaultVoice(
    "EnableDefaultVoice", category_Behavior, &VoiceChatService::getEnableDefaultVoice,
    &VoiceChatService::setEnableDefaultVoice, Reflection::PropertyDescriptor::STANDARD,
    Security::Plugin);
static Reflection::EnumPropDescriptor<VoiceChatService, Enums::RolloutState>
    propEnableVoiceVolumeControls("EnableVoiceVolumeControls", category_Behavior,
        &VoiceChatService::getEnableVoiceVolumeControls,
        &VoiceChatService::setEnableVoiceVolumeControls,
        Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
static Reflection::EnumPropDescriptor<VoiceChatService, Enums::AudioApiRollout>
    propUseAudioApi("UseAudioApi", category_Behavior, &VoiceChatService::getUseAudioApi,
        &VoiceChatService::setUseAudioApi, Reflection::PropertyDescriptor::STANDARD,
        Security::Plugin);
static Reflection::PropDescriptor<VoiceChatService, bool> propUseNewAudioApi(
    "UseNewAudioApi", category_Data, &VoiceChatService::getUseNewAudioApi,
    &VoiceChatService::setUseNewAudioApi, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING,
    Security::RobloxScript);
static Reflection::PropDescriptor<VoiceChatService, bool> propUseNewControlPaths(
    "UseNewControlPaths", category_Data, &VoiceChatService::getUseNewControlPaths,
    &VoiceChatService::setUseNewControlPaths, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING,
    Security::Roblox);
static Reflection::PropDescriptor<VoiceChatService, bool> propUseNewJoinFlow(
    "UseNewJoinFlow", category_Data, &VoiceChatService::getUseNewJoinFlow,
    &VoiceChatService::setUseNewJoinFlow, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING,
    Security::Roblox);
static Reflection::PropDescriptor<VoiceChatService, bool> propUseStreamSwitching(
    "UseStreamSwitching", category_Data, &VoiceChatService::getUseStreamSwitching,
    &VoiceChatService::setUseStreamSwitching, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING,
    Security::Roblox);
static Reflection::PropDescriptor<VoiceChatService, bool> propVoiceChatEnabledForPlaceOnRcc(
    "VoiceChatEnabledForPlaceOnRcc", category_State,
    &VoiceChatService::getVoiceChatEnabledForPlaceOnRcc,
    &VoiceChatService::setVoiceChatEnabledForPlaceOnRcc,
    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<VoiceChatService, bool> propVoiceChatEnabledForUniverseOnRcc(
    "VoiceChatEnabledForUniverseOnRcc", category_State,
    &VoiceChatService::getVoiceChatEnabledForUniverseOnRcc,
    &VoiceChatService::setVoiceChatEnabledForUniverseOnRcc,
    Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);

static Reflection::BoundFuncDesc<VoiceChatService, std::string()> funcGetInternalChannelId(
    &VoiceChatService::getInternalChannelId, "getInternalChannelId", Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, std::string()> funcGetInternalGroupId(
    &VoiceChatService::getInternalGroupId, "getInternalGroupId", Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, bool()> funcGetInternalPublishPause(
    &VoiceChatService::getInternalPublishPause, "getInternalPublishPause", Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, std::string()> funcGetInternalSessionId(
    &VoiceChatService::getInternalSessionId, "getInternalSessionId", Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, bool(long long)> funcGetInternalSubscribePause(
    &VoiceChatService::getInternalSubscribePause, "getInternalSubscribePause", "userId",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, bool()> funcGetInternalSubscribePauseAll(
    &VoiceChatService::getInternalSubscribePauseAll, "getInternalSubscribePauseAll",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, int()> funcGetInternalVoiceChatApiVersion(
    &VoiceChatService::getInternalVoiceChatApiVersion, "getInternalVoiceChatApiVersion",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, bool()> funcIsInternalPublishPaused(
    &VoiceChatService::isInternalPublishPaused, "isInternalPublishPaused",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, void()> funcJoinVoice(
    &VoiceChatService::joinVoice, "joinVoice", Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, shared_ptr<const Reflection::ValueTable>()>
    funcLastVoiceChatStats(&VoiceChatService::lastVoiceChatStats, "lastVoiceChatStats",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, void(Enums::VoiceClientLeaveReasons)>
    funcLeaveVoice(&VoiceChatService::leaveVoice, "leaveVoice", "leaveReason",
        Enums::VOICE_LEAVE_LUA_INITIATED, Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, void()> funcNotifyServerACSCleanup(
    &VoiceChatService::notifyServerACSCleanup, "notifyServerACSCleanup",
    Security::RobloxScript);
static Reflection::BoundFuncDesc<VoiceChatService, void()> funcRejoinVoice(
    &VoiceChatService::rejoinVoice, "rejoinVoice", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<VoiceChatService,
    shared_ptr<const Reflection::ValueArray>(shared_ptr<const Instances>)>
        funcGetChatGroupsAsync(&VoiceChatService::getChatGroupsAsync,
            "GetChatGroupsAsync", "players", Security::None);
static Reflection::BoundYieldFuncDesc<VoiceChatService, bool(long long)>
    funcIsVoiceEnabledForUserIdAsync(&VoiceChatService::isVoiceEnabledForUserIdAsync,
        "IsVoiceEnabledForUserIdAsync", "userId", Security::None);
static Reflection::EventDesc<VoiceChatService, void()> eventVoiceChatStatsCollected(
    &VoiceChatService::voiceChatStatsCollectedSignal, "VoiceChatStatsCollected",
    Security::RobloxScript);
REFLECTION_END();

VoiceChatService::VoiceChatService()
    : Service(true)
    , defaultDistanceAttenuation(Enums::VOICE_DISTANCE_INVERSE)
    , enableVoiceVolumeControls(Enums::ROLLOUT_DEFAULT)
    , useAudioApi(Enums::AUDIO_API_AUTOMATIC)
    , enableDefaultVoice(false)
    , useNewAudioApi(false)
    , useNewControlPaths(false)
    , useNewJoinFlow(false)
    , useStreamSwitching(false)
    , voiceChatEnabledForPlaceOnRcc(false)
    , voiceChatEnabledForUniverseOnRcc(false)
    , joined(false)
    , publishPaused(false)
    , subscribePausedAll(false)
    , lastLeaveReason(Enums::VOICE_LEAVE_UNKNOWN)
{
    setName(sVoiceChatService);
    setRobloxLocked(true);
}

#define RBX_VOICE_PROPERTY(Type, Name, field, descriptor) \
Type VoiceChatService::get##Name() const { return field; } \
void VoiceChatService::set##Name(Type value) { if (field != value) { field = value; raisePropertyChanged(descriptor); } }
RBX_VOICE_PROPERTY(Enums::VoiceChatDistanceAttenuationType, DefaultDistanceAttenuation, defaultDistanceAttenuation, propDefaultDistanceAttenuation)
RBX_VOICE_PROPERTY(bool, EnableDefaultVoice, enableDefaultVoice, propEnableDefaultVoice)
RBX_VOICE_PROPERTY(Enums::RolloutState, EnableVoiceVolumeControls, enableVoiceVolumeControls, propEnableVoiceVolumeControls)
RBX_VOICE_PROPERTY(Enums::AudioApiRollout, UseAudioApi, useAudioApi, propUseAudioApi)
RBX_VOICE_PROPERTY(bool, UseNewAudioApi, useNewAudioApi, propUseNewAudioApi)
RBX_VOICE_PROPERTY(bool, UseNewControlPaths, useNewControlPaths, propUseNewControlPaths)
RBX_VOICE_PROPERTY(bool, UseNewJoinFlow, useNewJoinFlow, propUseNewJoinFlow)
RBX_VOICE_PROPERTY(bool, UseStreamSwitching, useStreamSwitching, propUseStreamSwitching)
RBX_VOICE_PROPERTY(bool, VoiceChatEnabledForPlaceOnRcc, voiceChatEnabledForPlaceOnRcc, propVoiceChatEnabledForPlaceOnRcc)
RBX_VOICE_PROPERTY(bool, VoiceChatEnabledForUniverseOnRcc, voiceChatEnabledForUniverseOnRcc, propVoiceChatEnabledForUniverseOnRcc)
#undef RBX_VOICE_PROPERTY

std::string VoiceChatService::getInternalChannelId() { return channelId; }
std::string VoiceChatService::getInternalGroupId() { return groupId; }
bool VoiceChatService::getInternalPublishPause() { return publishPaused; }
std::string VoiceChatService::getInternalSessionId() { return sessionId; }
bool VoiceChatService::getInternalSubscribePause(long long userId)
{ return subscribePausedAll || subscribePaused[userId]; }
bool VoiceChatService::getInternalSubscribePauseAll() { return subscribePausedAll; }
int VoiceChatService::getInternalVoiceChatApiVersion() { return useNewAudioApi ? 2 : 1; }
bool VoiceChatService::isInternalPublishPaused() { return publishPaused; }

void VoiceChatService::joinVoice()
{
    if (joined)
        return;
    joined = true;
    Guid::generateRBXGUID(sessionId);
    channelId = sessionId;
    groupId = sessionId;
}

shared_ptr<const Reflection::ValueTable> VoiceChatService::lastVoiceChatStats()
{
    shared_ptr<Reflection::ValueTable> stats(new Reflection::ValueTable());
    (*stats)["joined"] = Reflection::Variant(joined);
    (*stats)["publishPaused"] = Reflection::Variant(publishPaused);
    (*stats)["subscribePausedAll"] = Reflection::Variant(subscribePausedAll);
    (*stats)["sessionId"] = Reflection::Variant(sessionId);
    (*stats)["leaveReason"] = Reflection::Variant(static_cast<int>(lastLeaveReason));
    voiceChatStatsCollectedSignal();
    return stats;
}

void VoiceChatService::leaveVoice(Enums::VoiceClientLeaveReasons reason)
{
    joined = false;
    lastLeaveReason = reason;
    channelId.clear();
    groupId.clear();
    sessionId.clear();
}

void VoiceChatService::notifyServerACSCleanup()
{
    subscribePaused.clear();
    subscribePausedAll = false;
}

void VoiceChatService::rejoinVoice()
{
    leaveVoice(Enums::VOICE_LEAVE_REJOIN_RECEIVED);
    joinVoice();
}

void VoiceChatService::getChatGroupsAsync(shared_ptr<const Instances> players,
    boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    shared_ptr<Reflection::ValueArray> groups(new Reflection::ValueArray());
    if (players && !players->empty())
    {
        shared_ptr<Reflection::ValueArray> group(new Reflection::ValueArray());
        for (Instances::const_iterator it = players->begin(); it != players->end(); ++it)
            group->push_back(Reflection::Variant(*it));
        groups->push_back(Reflection::Variant(shared_ptr<const Reflection::ValueArray>(group)));
    }
    resumeFunction(groups);
}

void VoiceChatService::isVoiceEnabledForUserIdAsync(long long userId,
    boost::function<void(bool)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    resumeFunction(userId > 0 && enableDefaultVoice &&
        (voiceChatEnabledForPlaceOnRcc || voiceChatEnabledForUniverseOnRcc));
}

} // namespace RBX
