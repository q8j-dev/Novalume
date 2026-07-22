#pragma once

#include "v8datamodel/InteractionEnums.h"
#include "v8tree/Service.h"

#include "rbx/signal.h"

#include <map>
#include <string>

namespace RBX {

extern const char* const sAvatarChatService;

class AvatarChatService
    : public DescribedNonCreatable<AvatarChatService, Instance, sAvatarChatService>
    , public Service
{
public:
    AvatarChatService();

    int getClientFeatures() const { return clientFeatures; }
    bool getClientFeaturesInitialized() const { return clientFeaturesInitialized; }
    int getServerFeatures() const { return serverFeatures; }
    void setServerFeatures(int value);

    long long debugCounterGet(std::string label, long long playerId);
    bool enableVoice();
    bool isEnabled(int mask, Enums::AvatarChatServiceFeature feature);
    bool isPlaceEnabled();
    bool isUniverseEnabled();
    int pollClientFeatures() { return clientFeatures; }
    int pollServerFeatures() { return serverFeatures; }
    bool deviceMeetsRequirementsForFeature(Enums::DeviceFeatureType feature);
    void getClientFeaturesAsync(boost::function<void(int)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void getServerFeaturesAsync(boost::function<void(int)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    void setClientFeatures(int value);
    void setDeviceFeatureAvailable(Enums::DeviceFeatureType feature, bool available);

    rbx::signal<void()> enableVoiceRequested;

private:
    int clientFeatures;
    int serverFeatures;
    int deviceFeatures;
    bool clientFeaturesInitialized;
    std::map<std::pair<std::string, long long>, long long> debugCounters;
};

} // namespace RBX
