#include "CullableSceneNode.h"

#include "SceneManager.h"
#include "Util.h"
#include "SpatialHashedScene.h"

#include "VisualEngine.h"
#include "GfxBase/FrameRateManager.h"

namespace RBX
{
namespace Graphics
{

CullableSceneNode::CullableSceneNode(VisualEngine* visualEngine, CullMode cullMode, unsigned int flags,
    SceneManager* sceneManager, const void* renderWorld)
    : visualEngine(visualEngine)
	, sceneManager(sceneManager ? sceneManager : visualEngine->getSceneManager())
	, renderWorld(renderWorld)
    , cullMode(cullMode)
    , flags(flags)
    , blockCount(1)
    , sqDistanceToFocus(0)
{
}

CullableSceneNode::~CullableSceneNode()
{
	sceneManager->getSpatialHashedScene()->internalRemoveChild(this);
}

bool CullableSceneNode::updateIsCulledByFRM()
{
	if (worldBounds.isNull())
        return true;

    const Vector3& focusPosition = sceneManager->getPointOfInterest();

    sqDistanceToFocus = G3D::ClosestSqDistanceToAABB(focusPosition, worldBounds.center(), worldBounds.size() * 0.5f);

    // count blocks and update farplane regardless of cullability.
    if (sqDistanceToFocus > 1e-3f)
        sceneManager->processSqPartDistance(sqDistanceToFocus);

    RBX::FrameRateManager* frm = visualEngine->getFrameRateManager();

	frm->AddBlockQuota(blockCount, sqDistanceToFocus, IsInSpatialHash());

	// We don't do distance-cull on huge objects
	if (cullMode == CullMode_SpatialHash && !IsInSpatialHash())
		return false;
	else
		return sqDistanceToFocus > frm->GetRenderCullSqDistance();
}

void CullableSceneNode::updateRenderQueue(RenderQueue& queue, const RenderCamera& camera, RenderQueue::Pass pass)
{
}

void CullableSceneNode::updateWorldBounds(const Extents& aabb)
{
    worldBounds = aabb;

    if (aabb.isNull())
		sceneManager->getSpatialHashedScene()->internalRemoveChild(this);
	else
		sceneManager->getSpatialHashedScene()->internalUpdateChild(this);
}

}
}
