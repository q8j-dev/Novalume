#pragma once

#include "v8datamodel/PartInstance.h"
#include "util/MeshId.h"
#include "util/TextureId.h"

namespace RBX
{

extern const char* const sMeshPart;

class MeshPart final
    : public DescribedCreatable<MeshPart, PartInstance, sMeshPart>
{
public:
    enum RenderFidelity
    {
        RenderFidelity_Automatic = 0,
        RenderFidelity_Precise = 1,
        RenderFidelity_Performance = 2,
    };

    MeshPart();

    const MeshId& getMeshId() const { return meshId; }
    void setMeshId(const MeshId& value);

    const TextureId& getTextureId() const { return textureId; }
    void setTextureId(const TextureId& value);

    bool getDoubleSided() const { return doubleSided; }
    void setDoubleSided(bool value);

    RenderFidelity getRenderFidelity() const { return renderFidelity; }
    void setRenderFidelity(RenderFidelity value);

    const Vector3& getInitialSize() const { return initialSize; }
    void setInitialSize(const Vector3& value);

    bool getHasSkinnedMesh() const { return hasSkinnedMesh; }
    void setHasSkinnedMesh(bool value);

private:
    MeshId meshId;
    TextureId textureId;
    bool doubleSided;
    RenderFidelity renderFidelity;
    Vector3 initialSize;
    bool hasSkinnedMesh;
};

} // namespace RBX
