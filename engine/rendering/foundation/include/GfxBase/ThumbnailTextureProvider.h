#pragma once

#include "GfxBase/ViewportTextureProvider.h"
#include "util/ContentId.h"

#include <cstdint>
#include <string>
#include <string_view>

namespace RBX {

enum class ThumbnailSceneType
{
    Avatar,
    AvatarBust,
    AvatarHeadShot
};

struct ThumbnailSceneRequest
{
    ThumbnailSceneType type = ThumbnailSceneType::Avatar;
    std::int64_t userId = 0;
    Vector2 size = Vector2(0.0f, 0.0f);
    std::string cacheKey;
};

bool parseThumbnailSceneRequest(std::string_view content,
    ThumbnailSceneRequest& request);
bool parseThumbnailSceneRequest(const ContentId& content,
    ThumbnailSceneRequest& request);

class ThumbnailSceneProvider
{
public:
    virtual ~ThumbnailSceneProvider() = default;
    virtual bool resolveThumbnailScene(const ThumbnailSceneRequest& request,
        ViewportTextureRequest& viewportRequest) = 0;
};

} // namespace RBX
