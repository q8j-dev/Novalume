#pragma once

#include "V8Tree/Service.h"
#include "V8Tree/Instance.h"

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
