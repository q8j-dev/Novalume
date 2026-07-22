#pragma once

#include "v8tree/Service.h"
#include "v8tree/Instance.h"

#include <cstdint>

namespace RBX {

extern const char* const sAvatarEditorService;

class AvatarEditorService
    : public DescribedCreatable<AvatarEditorService, Instance, sAvatarEditorService,
        Reflection::ClassDescriptor::INTERNAL_LOCAL>
    , public Service
{
public:
    AvatarEditorService();

    void bustAvatarFetchCache();
    std::uint64_t getAvatarFetchCacheGeneration() const { return avatarFetchCacheGeneration; }

private:
    std::uint64_t avatarFetchCacheGeneration;
};

}
