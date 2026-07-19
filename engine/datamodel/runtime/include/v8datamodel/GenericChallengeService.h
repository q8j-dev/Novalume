#pragma once

#include "V8Tree/Service.h"

#include "rbx/signal.h"

namespace RBX {

extern const char* const sGenericChallengeService;

class GenericChallengeService
    : public DescribedNonCreatable<GenericChallengeService, Instance,
          sGenericChallengeService>
    , public Service
{
public:
    GenericChallengeService();

    void signalChallengeAbandoned(std::string challengeId);
    void signalChallengeCompleted(std::string challengeId,
        std::string challengeType, std::string challengeMetadata);
    void signalChallengeInvalidated(std::string challengeId);
    void signalChallengeLoaded(std::string challengeId, bool success);
    void signalChallengeRequired(std::string challengeId,
        std::string challengeType, std::string challengeMetadata);

    rbx::remote_signal<void(std::string)> challengeAbandonedSignal;
    rbx::remote_signal<void(std::string, std::string, std::string)>
        challengeCompletedSignal;
    rbx::remote_signal<void(std::string)> challengeInvalidatedSignal;
    rbx::remote_signal<void(std::string, bool)> challengeLoadedSignal;
    rbx::remote_signal<void(std::string, std::string, std::string)>
        challengeRequiredSignal;
};

} // namespace RBX
