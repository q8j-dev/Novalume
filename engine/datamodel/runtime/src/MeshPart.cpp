#include "v8datamodel/MeshPart.h"

namespace RBX
{

const char* const sMeshPart = "MeshPart";

namespace Reflection
{

template<>
EnumDesc<MeshPart::RenderFidelity>::EnumDesc()
    : EnumDescriptor("RenderFidelity")
{
    addPair(MeshPart::RenderFidelity_Automatic, "Automatic");
    addPair(MeshPart::RenderFidelity_Precise, "Precise");
    addPair(MeshPart::RenderFidelity_Performance, "Performance");
}

} // namespace Reflection

REFLECTION_BEGIN();
static Reflection::PropDescriptor<MeshPart, MeshId> propMeshId(
    "MeshId", category_Appearance, &MeshPart::getMeshId, &MeshPart::setMeshId);
static Reflection::PropDescriptor<MeshPart, TextureId> propTextureId(
    "TextureID", category_Appearance, &MeshPart::getTextureId, &MeshPart::setTextureId);
static Reflection::PropDescriptor<MeshPart, bool> propDoubleSided(
    "DoubleSided", category_Appearance, &MeshPart::getDoubleSided, &MeshPart::setDoubleSided);
static Reflection::EnumPropDescriptor<MeshPart, MeshPart::RenderFidelity> propRenderFidelity(
    "RenderFidelity", category_Appearance,
    &MeshPart::getRenderFidelity, &MeshPart::setRenderFidelity);
static Reflection::PropDescriptor<MeshPart, Vector3> propInitialSize(
    "InitialSize", category_Data, &MeshPart::getInitialSize, &MeshPart::setInitialSize,
    Reflection::PropertyDescriptor::STREAMING);
static Reflection::PropDescriptor<MeshPart, bool> propHasSkinnedMesh(
    "HasSkinnedMesh", category_Data,
    &MeshPart::getHasSkinnedMesh, &MeshPart::setHasSkinnedMesh,
    Reflection::PropertyDescriptor::STREAMING);
REFLECTION_END();

MeshPart::MeshPart()
    : doubleSided(false)
    , renderFidelity(RenderFidelity_Automatic)
    , initialSize(1.0f, 1.0f, 1.0f)
    , hasSkinnedMesh(false)
{
    setName("MeshPart");
}

void MeshPart::setMeshId(const MeshId& value)
{
    if (meshId != value)
    {
        meshId = value;
        raisePropertyChanged(propMeshId);
    }
}

void MeshPart::setTextureId(const TextureId& value)
{
    if (textureId != value)
    {
        textureId = value;
        raisePropertyChanged(propTextureId);
    }
}

void MeshPart::setDoubleSided(bool value)
{
    if (doubleSided != value)
    {
        doubleSided = value;
        raisePropertyChanged(propDoubleSided);
    }
}

void MeshPart::setRenderFidelity(RenderFidelity value)
{
    if (renderFidelity != value)
    {
        renderFidelity = value;
        raisePropertyChanged(propRenderFidelity);
    }
}

void MeshPart::setInitialSize(const Vector3& value)
{
    if (initialSize != value)
    {
        initialSize = value;
        raisePropertyChanged(propInitialSize);
    }
}

void MeshPart::setHasSkinnedMesh(bool value)
{
    if (hasSkinnedMesh != value)
    {
        hasSkinnedMesh = value;
        raisePropertyChanged(propHasSkinnedMesh);
    }
}

} // namespace RBX
