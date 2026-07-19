#pragma once

#include "GfxBase/TextureProxyBase.h"
#include "Util/G3DCore.h"
#include "boost/shared_ptr.hpp"
#include <string>

namespace RBX {

class Camera;
class Instance;

struct ViewportTextureRequest
{
    boost::shared_ptr<Instance> world;
    const Camera* camera;
    CoordinateFrame cameraCFrame;
    float fieldOfView;
    Vector2 size;
    Color3 ambient;
    Color3 lightColor;
    Vector3 lightDirection;
    bool mirrored;
    std::string cacheKey;
};

class ViewportTextureProvider
{
public:
    virtual ~ViewportTextureProvider() {}
    virtual TextureProxyBaseRef requestViewportTexture(const ViewportTextureRequest& request) = 0;
    virtual bool requiresViewportTextureFlipping() const = 0;
};

} // namespace RBX
