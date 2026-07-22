#pragma once

#include "v8datamodel/InteractionEnums.h"
#include "v8tree/Service.h"

#include "rbx/signal.h"

#include <map>
#include <string>

namespace RBX {

extern const char* const sVoiceChatService;

class VoiceChatService
    : public DescribedNonCreatable<VoiceChatService, Instance, sVoiceChatService>
    , public Service
{
public:
    VoiceChatService();

    Enums::VoiceChatDistanceAttenuationType getDefaultDistanceAttenuation() const;
    void setDefaultDistanceAttenuation(Enums::VoiceChatDistanceAttenuationType value);
    bool getEnableDefaultVoice() const;
    void setEnableDefaultVoice(bool value);
    Enums::RolloutState getEnableVoiceVolumeControls() const;
    void setEnableVoiceVolumeControls(Enums::RolloutState value);
    Enums::AudioApiRollout getUseAudioApi() const;
    void setUseAudioApi(Enums::AudioApiRollout value);
    bool getUseNewAudioApi() const;
    void setUseNewAudioApi(bool value);
    bool getUseNewControlPaths() const;
    void setUseNewControlPaths(bool value);
    bool getUseNewJoinFlow() const;
    void setUseNewJoinFlow(bool value);
    bool getUseStreamSwitching() const;
    void setUseStreamSwitching(bool value);
    bool getVoiceChatEnabledForPlaceOnRcc() const;
    void setVoiceChatEnabledForPlaceOnRcc(bool value);
    bool getVoiceChatEnabledForUniverseOnRcc() const;
    void setVoiceChatEnabledForUniverseOnRcc(bool value);

    std::string getInternalChannelId();
    std::string getInternalGroupId();
    bool getInternalPublishPause();
    std::string getInternalSessionId();
    bool getInternalSubscribePause(long long userId);
    bool getInternalSubscribePauseAll();
    int getInternalVoiceChatApiVersion();
    bool isInternalPublishPaused();
    void joinVoice();
    shared_ptr<const Reflection::ValueTable> lastVoiceChatStats();
    void leaveVoice(Enums::VoiceClientLeaveReasons reason);
    void notifyServerACSCleanup();
    void rejoinVoice();

    void getChatGroupsAsync(shared_ptr<const Instances> players,
        boost::function<void(shared_ptr<const Reflection::ValueArray>)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void isVoiceEnabledForUserIdAsync(long long userId,
        boost::function<void(bool)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    rbx::signal<void()> voiceChatStatsCollectedSignal;

private:
    Enums::VoiceChatDistanceAttenuationType defaultDistanceAttenuation;
    Enums::RolloutState enableVoiceVolumeControls;
    Enums::AudioApiRollout useAudioApi;
    bool enableDefaultVoice;
    bool useNewAudioApi;
    bool useNewControlPaths;
    bool useNewJoinFlow;
    bool useStreamSwitching;
    bool voiceChatEnabledForPlaceOnRcc;
    bool voiceChatEnabledForUniverseOnRcc;
    bool joined;
    bool publishPaused;
    bool subscribePausedAll;
    std::map<long long, bool> subscribePaused;
    std::string channelId;
    std::string groupId;
    std::string sessionId;
    Enums::VoiceClientLeaveReasons lastLeaveReason;
};

} // namespace RBX
