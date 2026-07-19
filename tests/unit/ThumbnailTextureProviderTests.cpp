#include "GfxBase/ThumbnailTextureProvider.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main()
{
    RBX::ThumbnailSceneRequest request;
    require(RBX::parseThumbnailSceneRequest(
        "rbxthumb://type=Avatar&id=1&w=150&h=150", request),
        "current Avatar thumbnail URI must parse");
    require(request.type == RBX::ThumbnailSceneType::Avatar,
        "Avatar URI must preserve its scene type");
    require(request.userId == 1 && request.size == RBX::Vector2(150.0f, 150.0f),
        "Avatar URI must preserve user and dimensions");

    require(RBX::parseThumbnailSceneRequest(
        "rbxthumb://type=AvatarBust&id=42&w=100&h=150", request),
        "AvatarBust thumbnail URI must parse");
    require(request.type == RBX::ThumbnailSceneType::AvatarBust,
        "AvatarBust URI must preserve its scene type");

    require(RBX::parseThumbnailSceneRequest(
        "rbxthumb://type=AvatarHeadShot&id=42&w=48&h=48", request),
        "AvatarHeadShot thumbnail URI must parse");
    require(request.type == RBX::ThumbnailSceneType::AvatarHeadShot,
        "AvatarHeadShot URI must preserve its scene type");

    require(!RBX::parseThumbnailSceneRequest(
        "rbxasset://textures/ui/icon.png", request),
        "non-thumbnail content must not parse as a thumbnail");
    require(!RBX::parseThumbnailSceneRequest(
        "rbxthumb://type=Avatar&id=1&w=0&h=150", request),
        "zero-sized thumbnails must be rejected");
    require(!RBX::parseThumbnailSceneRequest(
        "rbxthumb://type=Avatar&id=1&id=2&w=150&h=150", request),
        "duplicate thumbnail parameters must be rejected");
    require(!RBX::parseThumbnailSceneRequest(
        "rbxthumb://type=Asset&id=1&w=150&h=150", request),
        "unsupported scene types must not masquerade as avatar thumbnails");

    return 0;
}
