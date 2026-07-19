#include "V8DataModel/Bone.h"
#include "V8DataModel/PartInstance.h"

namespace RBX
{

const char* const sBone = "Bone";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<Bone, CoordinateFrame> propTransform(
    "Transform", category_Data, &Bone::getTransform, &Bone::setTransform,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<Bone, CoordinateFrame> propTransformedCFrame(
    "TransformedCFrame", "Derived Data", &Bone::getTransformedCFrame, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<Bone, CoordinateFrame> propTransformedWorldCFrame(
    "TransformedWorldCFrame", "Derived Data", &Bone::getTransformedWorldCFrame,
    NULL, Reflection::PropertyDescriptor::SCRIPTING);
REFLECTION_END();

Bone::Bone()
    : transform()
{
    setName("Bone");
}

void Bone::setTransform(const CoordinateFrame& value)
{
    if (transform != value)
    {
        transform = value;
        raisePropertyChanged(propTransform);
        raisePropertyChanged(propTransformedCFrame);
        raisePropertyChanged(propTransformedWorldCFrame);
    }
}

CoordinateFrame Bone::getTransformedCFrame() const
{
    return getFrameInPart() * transform;
}

CoordinateFrame Bone::getTransformedWorldCFrame() const
{
    return getParentFrame() * getTransformedCFrame();
}

CoordinateFrame Bone::getParentFrame() const
{
    if (const Bone* bone = Instance::fastDynamicCast<const Bone>(getParent()))
        return bone->getTransformedWorldCFrame();
    if (const PartInstance* part =
            Instance::fastDynamicCast<const PartInstance>(getParent()))
        return part->getCoordinateFrame();
    return CoordinateFrame();
}

const std::string& Bone::getParentName()
{
    if (const Instance* parent = getParent())
        return parent->getName();
    return IAnimatableJoint::sNULL;
}

const std::string& Bone::getPartName()
{
    return getName();
}

void Bone::applyPose(const CachedPose& pose)
{
    if (!pose.initialized)
    {
        setTransform(CoordinateFrame());
        return;
    }

    CachedPose weighted = pose;
    weighted.translation *= pose.weight;
    weighted.rotaxisangle *= pose.weight;
    setTransform(weighted.getCFrame());
}

void Bone::verifySetParent(const Instance* instance) const
{
    Instance::verifySetParent(instance);
    if (instance != nullptr &&
        Instance::fastDynamicCast<const PartInstance>(instance) == nullptr &&
        Instance::fastDynamicCast<const Bone>(instance) == nullptr)
        throw RBX::runtime_error("Bones can only be parented to parts or bones");
}

} // namespace RBX
