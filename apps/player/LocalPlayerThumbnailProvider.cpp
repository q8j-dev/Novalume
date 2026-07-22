#include "LocalPlayerThumbnailProvider.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/ModelInstance.h"
#include "v8datamodel/PartInstance.h"
#include "network/Player.h"
#include "network/Players.h"
#include "util/Extents.h"

#include <algorithm>
#include <cmath>

namespace rbx::player {
namespace {

RBX::Extents selectFramingExtents(RBX::ModelInstance& character,
    RBX::ThumbnailSceneType type)
{
    const RBX::Extents characterExtents = character.computeExtentsWorld();
    if (type == RBX::ThumbnailSceneType::Avatar)
        return characterExtents;

    if (type == RBX::ThumbnailSceneType::AvatarHeadShot)
    {
        if (RBX::PartInstance* head = RBX::Instance::fastDynamicCast<RBX::PartInstance>(
                character.findFirstChildByName("Head")))
        {
            const RBX::Extents headExtents = head->computeExtentsWorld();
            const RBX::Vector3 padding = headExtents.size() * 0.35f;
            return RBX::Extents(headExtents.min() - padding,
                headExtents.max() + padding);
        }
    }

    const float height = characterExtents.size().y;
    RBX::Vector3 lower = characterExtents.min();
    lower.y += height * 0.35f;
    return RBX::Extents(lower, characterExtents.max());
}

RBX::CoordinateFrame frameExtents(const RBX::Extents& extents,
    const RBX::Vector2& outputSize, float fieldOfViewDegrees)
{
    const RBX::Vector3 size = extents.size();
    const float aspect = std::max(0.01f, outputSize.x / outputSize.y);
    const float tangent = std::tan(G3D::toRadians(fieldOfViewDegrees) * 0.5f);
    const float verticalDistance = size.y * 0.5f / tangent;
    const float horizontalDistance = size.x * 0.5f / (tangent * aspect);
    const float distance =
        (std::max(verticalDistance, horizontalDistance) + size.z * 0.5f) * 1.12f;

    const RBX::Vector3 target = extents.center();
    RBX::CoordinateFrame camera(target + RBX::Vector3(0.0f, 0.0f, -distance));
    camera.lookAt(target);
    return camera;
}

} // namespace

LocalPlayerThumbnailProvider::LocalPlayerThumbnailProvider(
    const boost::shared_ptr<RBX::DataModel>& dataModel)
    : dataModel(dataModel)
{
}

bool LocalPlayerThumbnailProvider::resolveThumbnailScene(
    const RBX::ThumbnailSceneRequest& request,
    RBX::ViewportTextureRequest& viewportRequest)
{
    boost::shared_ptr<RBX::DataModel> model = dataModel.lock();
    if (!model)
        return false;

    RBX::Network::Players* players =
        RBX::ServiceProvider::find<RBX::Network::Players>(model.get());
    RBX::Network::Player* player = players ? players->getLocalPlayer() : nullptr;
    if (!player || player->getUserID() != request.userId || !player->getCharacter())
        return false;

    boost::shared_ptr<RBX::ModelInstance> character = player->getSharedCharacter();
    if (!character)
        return false;

    const RBX::Extents extents = selectFramingExtents(*character, request.type);
    if (extents.isNull() || extents.isNanInf() || extents.size().squaredMagnitude() < 0.001f)
        return false;

    constexpr float fieldOfView = 30.0f;
    viewportRequest.world = boost::static_pointer_cast<RBX::Instance>(character);
    viewportRequest.camera = nullptr;
    viewportRequest.cameraCFrame = frameExtents(extents, request.size, fieldOfView);
    viewportRequest.fieldOfView = fieldOfView;
    viewportRequest.size = request.size;
    viewportRequest.ambient = RBX::Color3(0.58f, 0.58f, 0.58f);
    viewportRequest.lightColor = RBX::Color3::white();
    viewportRequest.lightDirection = RBX::Vector3(-1.0f, -1.0f, -1.0f).unit();
    viewportRequest.mirrored = false;
    viewportRequest.cacheKey = request.cacheKey;
    return true;
}

} // namespace rbx::player
