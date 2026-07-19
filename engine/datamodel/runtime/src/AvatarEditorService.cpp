#include "v8datamodel/AvatarEditorService.h"

namespace RBX {

const char* const sAvatarEditorService = "AvatarEditorService";

static Reflection::BoundFuncDesc<AvatarEditorService, void()> funcBustAvatarFetchCache(
    &AvatarEditorService::bustAvatarFetchCache, "BustAvatarFetchCache", Security::None);

AvatarEditorService::AvatarEditorService()
    : avatarFetchCacheGeneration(0)
{
    setName(sAvatarEditorService);
}

void AvatarEditorService::bustAvatarFetchCache()
{
    ++avatarFetchCacheGeneration;
}

}
