#include "V8DataModel/ModernAvatar.h"

#include "V8DataModel/MeshPart.h"
#include "V8DataModel/Workspace.h"
#include "V8World/Assembly.h"
#include "V8World/Motor6DJoint.h"
#include "V8World/MotorJoint.h"
#include "V8World/RotateJoint.h"
#include "V8World/Primitive.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace RBX
{

const char* const sAnimationConstraint = "AnimationConstraint";
const char* const sBallSocketConstraint = "BallSocketConstraint";
const char* const sHingeConstraint = "HingeConstraint";
const char* const sNoCollisionConstraint = "NoCollisionConstraint";
const char* const sWrapTarget = "WrapTarget";
const char* const sFaceControls = "FaceControls";
const char* const sHumanoidDescription = "HumanoidDescription";

namespace Reflection
{
template<>
EnumDesc<HingeConstraint::ActuatorType>::EnumDesc()
    : EnumDescriptor("ActuatorType")
{
    addPair(HingeConstraint::ACTUATOR_NONE, "None");
    addPair(HingeConstraint::ACTUATOR_MOTOR, "Motor");
    addPair(HingeConstraint::ACTUATOR_SERVO, "Servo");
}
}

REFLECTION_BEGIN();
static Reflection::PropDescriptor<HumanoidDescription, std::string>
    humanoidDescriptionBackAccessory("BackAccessory", category_Appearance,
        &HumanoidDescription::getBackAccessory,
        &HumanoidDescription::setBackAccessory);
static Reflection::PropDescriptor<HumanoidDescription, std::string>
    humanoidDescriptionFaceAccessory("FaceAccessory", category_Appearance,
        &HumanoidDescription::getFaceAccessory,
        &HumanoidDescription::setFaceAccessory);
static Reflection::PropDescriptor<HumanoidDescription, std::string>
    humanoidDescriptionFrontAccessory("FrontAccessory", category_Appearance,
        &HumanoidDescription::getFrontAccessory,
        &HumanoidDescription::setFrontAccessory);
static Reflection::PropDescriptor<HumanoidDescription, std::string>
    humanoidDescriptionHairAccessory("HairAccessory", category_Appearance,
        &HumanoidDescription::getHairAccessory,
        &HumanoidDescription::setHairAccessory);
static Reflection::PropDescriptor<HumanoidDescription, std::string>
    humanoidDescriptionHatAccessory("HatAccessory", category_Appearance,
        &HumanoidDescription::getHatAccessory,
        &HumanoidDescription::setHatAccessory);
static Reflection::PropDescriptor<HumanoidDescription, std::string>
    humanoidDescriptionNeckAccessory("NeckAccessory", category_Appearance,
        &HumanoidDescription::getNeckAccessory,
        &HumanoidDescription::setNeckAccessory);
static Reflection::PropDescriptor<HumanoidDescription, std::string>
    humanoidDescriptionShouldersAccessory("ShouldersAccessory", category_Appearance,
        &HumanoidDescription::getShouldersAccessory,
        &HumanoidDescription::setShouldersAccessory);
static Reflection::PropDescriptor<HumanoidDescription, std::string>
    humanoidDescriptionWaistAccessory("WaistAccessory", category_Appearance,
        &HumanoidDescription::getWaistAccessory,
        &HumanoidDescription::setWaistAccessory);
static Reflection::PropDescriptor<HumanoidDescription, float>
    humanoidDescriptionBodyTypeScale("BodyTypeScale", category_Appearance,
        &HumanoidDescription::getBodyTypeScale,
        &HumanoidDescription::setBodyTypeScale);
static Reflection::PropDescriptor<HumanoidDescription, float>
    humanoidDescriptionDepthScale("DepthScale", category_Appearance,
        &HumanoidDescription::getDepthScale,
        &HumanoidDescription::setDepthScale);
static Reflection::PropDescriptor<HumanoidDescription, float>
    humanoidDescriptionHeadScale("HeadScale", category_Appearance,
        &HumanoidDescription::getHeadScale,
        &HumanoidDescription::setHeadScale);
static Reflection::PropDescriptor<HumanoidDescription, float>
    humanoidDescriptionHeightScale("HeightScale", category_Appearance,
        &HumanoidDescription::getHeightScale,
        &HumanoidDescription::setHeightScale);
static Reflection::PropDescriptor<HumanoidDescription, float>
    humanoidDescriptionProportionScale("ProportionScale", category_Appearance,
        &HumanoidDescription::getProportionScale,
        &HumanoidDescription::setProportionScale);
static Reflection::PropDescriptor<HumanoidDescription, float>
    humanoidDescriptionWidthScale("WidthScale", category_Appearance,
        &HumanoidDescription::getWidthScale,
        &HumanoidDescription::setWidthScale);

static Reflection::RefPropDescriptor<AnimationConstraint, Attachment>
    animationAttachment0("Attachment0", category_Data,
        &AnimationConstraint::getAttachment0, &AnimationConstraint::setAttachment0);
static Reflection::RefPropDescriptor<AnimationConstraint, Attachment>
    animationAttachment1("Attachment1", category_Data,
        &AnimationConstraint::getAttachment1, &AnimationConstraint::setAttachment1);
static Reflection::PropDescriptor<AnimationConstraint, bool>
    animationEnabled("Enabled", category_Behavior,
        &AnimationConstraint::getEnabled, &AnimationConstraint::setEnabled);
static Reflection::PropDescriptor<AnimationConstraint, bool>
    animationIsKinematic("IsKinematic", category_Behavior,
        &AnimationConstraint::getIsKinematic, &AnimationConstraint::setIsKinematic);
static Reflection::PropDescriptor<AnimationConstraint, float>
    animationMaxForce("MaxForce", category_Behavior,
        &AnimationConstraint::getMaxForce, &AnimationConstraint::setMaxForce);
static Reflection::PropDescriptor<AnimationConstraint, float>
    animationMaxTorque("MaxTorque", category_Behavior,
        &AnimationConstraint::getMaxTorque, &AnimationConstraint::setMaxTorque);
static Reflection::PropDescriptor<AnimationConstraint, bool>
    animationVisible("Visible", category_Appearance,
        &AnimationConstraint::getVisible, &AnimationConstraint::setVisible);
static Reflection::PropDescriptor<AnimationConstraint, BrickColor>
    animationColor("Color", category_Appearance,
        &AnimationConstraint::getColor, &AnimationConstraint::setColor);
static Reflection::PropDescriptor<AnimationConstraint, CoordinateFrame>
    animationTransform("Transform", category_Data,
        &AnimationConstraint::getTransform, &AnimationConstraint::setTransform);

static Reflection::RefPropDescriptor<BallSocketConstraint, Attachment>
    ballAttachment0("Attachment0", category_Data,
        &BallSocketConstraint::getAttachment0, &BallSocketConstraint::setAttachment0);
static Reflection::RefPropDescriptor<BallSocketConstraint, Attachment>
    ballAttachment1("Attachment1", category_Data,
        &BallSocketConstraint::getAttachment1, &BallSocketConstraint::setAttachment1);
static Reflection::PropDescriptor<BallSocketConstraint, bool>
    ballEnabled("Enabled", category_Behavior,
        &BallSocketConstraint::getEnabled, &BallSocketConstraint::setEnabled);
static Reflection::PropDescriptor<BallSocketConstraint, bool>
    ballLimitsEnabled("LimitsEnabled", category_Behavior,
        &BallSocketConstraint::getLimitsEnabled, &BallSocketConstraint::setLimitsEnabled);
static Reflection::PropDescriptor<BallSocketConstraint, bool>
    ballTwistLimitsEnabled("TwistLimitsEnabled", category_Behavior,
        &BallSocketConstraint::getTwistLimitsEnabled,
        &BallSocketConstraint::setTwistLimitsEnabled);
static Reflection::PropDescriptor<BallSocketConstraint, float>
    ballUpperAngle("UpperAngle", category_Data,
        &BallSocketConstraint::getUpperAngle, &BallSocketConstraint::setUpperAngle);
static Reflection::PropDescriptor<BallSocketConstraint, float>
    ballTwistLowerAngle("TwistLowerAngle", category_Data,
        &BallSocketConstraint::getTwistLowerAngle,
        &BallSocketConstraint::setTwistLowerAngle);
static Reflection::PropDescriptor<BallSocketConstraint, float>
    ballTwistUpperAngle("TwistUpperAngle", category_Data,
        &BallSocketConstraint::getTwistUpperAngle,
        &BallSocketConstraint::setTwistUpperAngle);
static Reflection::PropDescriptor<BallSocketConstraint, float>
    ballRestitution("Restitution", category_Data,
        &BallSocketConstraint::getRestitution, &BallSocketConstraint::setRestitution);
static Reflection::PropDescriptor<BallSocketConstraint, float>
    ballMaxFrictionTorque("MaxFrictionTorqueXml", category_Data,
        &BallSocketConstraint::getMaxFrictionTorque,
        &BallSocketConstraint::setMaxFrictionTorque);
static Reflection::PropDescriptor<BallSocketConstraint, float>
    ballRadius("Radius", category_Appearance,
        &BallSocketConstraint::getRadius, &BallSocketConstraint::setRadius);
static Reflection::PropDescriptor<BallSocketConstraint, bool>
    ballVisible("Visible", category_Appearance,
        &BallSocketConstraint::getVisible, &BallSocketConstraint::setVisible);
static Reflection::PropDescriptor<BallSocketConstraint, BrickColor>
    ballColor("Color", category_Appearance,
        &BallSocketConstraint::getColor, &BallSocketConstraint::setColor);

static Reflection::RefPropDescriptor<HingeConstraint, Attachment>
    hingeAttachment0("Attachment0", category_Data,
        &HingeConstraint::getAttachment0, &HingeConstraint::setAttachment0);
static Reflection::RefPropDescriptor<HingeConstraint, Attachment>
    hingeAttachment1("Attachment1", category_Data,
        &HingeConstraint::getAttachment1, &HingeConstraint::setAttachment1);
static Reflection::PropDescriptor<HingeConstraint, bool>
    hingeEnabled("Enabled", category_Behavior,
        &HingeConstraint::getEnabled, &HingeConstraint::setEnabled);
static Reflection::EnumPropDescriptor<HingeConstraint, HingeConstraint::ActuatorType>
    hingeActuatorType("ActuatorType", category_Behavior,
        &HingeConstraint::getActuatorType, &HingeConstraint::setActuatorType);
static Reflection::PropDescriptor<HingeConstraint, bool>
    hingeLimitsEnabled("LimitsEnabled", category_Behavior,
        &HingeConstraint::getLimitsEnabled, &HingeConstraint::setLimitsEnabled);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeLowerAngle("LowerAngle", category_Data,
        &HingeConstraint::getLowerAngle, &HingeConstraint::setLowerAngle);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeUpperAngle("UpperAngle", category_Data,
        &HingeConstraint::getUpperAngle, &HingeConstraint::setUpperAngle);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeTargetAngle("TargetAngle", category_Data,
        &HingeConstraint::getTargetAngle, &HingeConstraint::setTargetAngle);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeAngularSpeed("AngularSpeed", category_Data,
        &HingeConstraint::getAngularSpeed, &HingeConstraint::setAngularSpeed);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeAngularVelocity("AngularVelocity", category_Data,
        &HingeConstraint::getAngularVelocity, &HingeConstraint::setAngularVelocity);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeAngularResponsiveness("AngularResponsiveness", category_Data,
        &HingeConstraint::getAngularResponsiveness,
        &HingeConstraint::setAngularResponsiveness);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeMotorMaxAcceleration("MotorMaxAcceleration", category_Data,
        &HingeConstraint::getMotorMaxAcceleration,
        &HingeConstraint::setMotorMaxAcceleration);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeMotorMaxTorque("MotorMaxTorque", category_Data,
        &HingeConstraint::getMotorMaxTorque, &HingeConstraint::setMotorMaxTorque);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeServoMaxTorque("ServoMaxTorque", category_Data,
        &HingeConstraint::getServoMaxTorque, &HingeConstraint::setServoMaxTorque);
static Reflection::PropDescriptor<HingeConstraint, bool>
    hingeSoftlock("SoftlockServoUponReachingTarget", category_Behavior,
        &HingeConstraint::getSoftlockServoUponReachingTarget,
        &HingeConstraint::setSoftlockServoUponReachingTarget);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeRestitution("Restitution", category_Data,
        &HingeConstraint::getRestitution, &HingeConstraint::setRestitution);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeRadius("Radius", category_Appearance,
        &HingeConstraint::getRadius, &HingeConstraint::setRadius);
static Reflection::PropDescriptor<HingeConstraint, bool>
    hingeVisible("Visible", category_Appearance,
        &HingeConstraint::getVisible, &HingeConstraint::setVisible);
static Reflection::PropDescriptor<HingeConstraint, BrickColor>
    hingeColor("Color", category_Appearance,
        &HingeConstraint::getColor, &HingeConstraint::setColor);
static Reflection::PropDescriptor<HingeConstraint, float>
    hingeCurrentAngle("CurrentAngle", category_Data,
        &HingeConstraint::getCurrentAngle, NULL,
        Reflection::PropertyDescriptor::UI);

static Reflection::RefPropDescriptor<NoCollisionConstraint, PartInstance>
    noCollisionPart0("Part0", category_Data,
        &NoCollisionConstraint::getPart0, &NoCollisionConstraint::setPart0);
static Reflection::RefPropDescriptor<NoCollisionConstraint, PartInstance>
    noCollisionPart1("Part1", category_Data,
        &NoCollisionConstraint::getPart1, &NoCollisionConstraint::setPart1);
static Reflection::PropDescriptor<NoCollisionConstraint, bool>
    noCollisionEnabled("Enabled", category_Behavior,
        &NoCollisionConstraint::getEnabled, &NoCollisionConstraint::setEnabled);

static Reflection::PropDescriptor<WrapTarget, MeshId>
    wrapCageMeshId("CageMeshId", category_Data,
        &WrapTarget::getCageMeshId, &WrapTarget::setCageMeshId);
static Reflection::PropDescriptor<WrapTarget, MeshId>
    wrapTemporaryCageMeshId("TemporaryCageMeshId", category_Data,
        &WrapTarget::getTemporaryCageMeshId, &WrapTarget::setTemporaryCageMeshId);
static Reflection::PropDescriptor<WrapTarget, ContentId>
    wrapHsrAssetId("HSRAssetId", category_Data,
        &WrapTarget::getHSRAssetId, &WrapTarget::setHSRAssetId);
static Reflection::PropDescriptor<WrapTarget, BinaryString>
	wrapHsrData("HSRData", category_Data,
		&WrapTarget::getHSRData, &WrapTarget::setHSRData);
static Reflection::PropDescriptor<WrapTarget, MeshId>
	wrapHsrMeshId("HSRMeshId", category_Data,
		&WrapTarget::getHSRMeshId, &WrapTarget::setHSRMeshId);
static Reflection::PropDescriptor<WrapTarget, CoordinateFrame>
    wrapCageOrigin("CageOrigin", category_Data,
        &WrapTarget::getCageOrigin, &WrapTarget::setCageOrigin);
static Reflection::PropDescriptor<WrapTarget, CoordinateFrame>
    wrapImportOrigin("ImportOrigin", category_Data,
        &WrapTarget::getImportOrigin, &WrapTarget::setImportOrigin);
static Reflection::PropDescriptor<WrapTarget, float>
    wrapStiffness("Stiffness", category_Data,
        &WrapTarget::getStiffness, &WrapTarget::setStiffness);

static Reflection::PropDescriptor<FaceControls, std::string>
    faceOverrideFacsData("InternalOverrideFACSData", category_Data,
        &FaceControls::getInternalOverrideFACSData,
        &FaceControls::setInternalOverrideFACSData,
        Reflection::PropertyDescriptor::STREAMING);
REFLECTION_END();

HumanoidDescription::HumanoidDescription()
    : bodyTypeScale(0.0f)
    , depthScale(1.0f)
    , headScale(1.0f)
    , heightScale(1.0f)
    , proportionScale(0.0f)
    , widthScale(1.0f)
{
    setName(sHumanoidDescription);
}

#define RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER(Name, member, descriptor) \
    void HumanoidDescription::set##Name(const std::string& value) \
    { \
        if (member != value) \
        { \
            member = value; \
            raisePropertyChanged(descriptor); \
        } \
    }

RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER(BackAccessory, backAccessory,
    humanoidDescriptionBackAccessory)
RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER(FaceAccessory, faceAccessory,
    humanoidDescriptionFaceAccessory)
RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER(FrontAccessory, frontAccessory,
    humanoidDescriptionFrontAccessory)
RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER(HairAccessory, hairAccessory,
    humanoidDescriptionHairAccessory)
RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER(HatAccessory, hatAccessory,
    humanoidDescriptionHatAccessory)
RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER(NeckAccessory, neckAccessory,
    humanoidDescriptionNeckAccessory)
RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER(ShouldersAccessory,
    shouldersAccessory, humanoidDescriptionShouldersAccessory)
RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER(WaistAccessory, waistAccessory,
    humanoidDescriptionWaistAccessory)

#undef RBX_DEFINE_HUMANOID_DESCRIPTION_STRING_SETTER

namespace
{
float finiteAvatarScale(float value, float fallback, float minimum, float maximum)
{
    return std::isfinite(value) ? std::max(minimum, std::min(maximum, value)) : fallback;
}
}

#define RBX_DEFINE_HUMANOID_DESCRIPTION_SCALE_SETTER(Name, member, descriptor, fallback, minimum, maximum) \
    void HumanoidDescription::set##Name(float value) \
    { \
        value = finiteAvatarScale(value, fallback, minimum, maximum); \
        if (member != value) \
        { \
            member = value; \
            raisePropertyChanged(descriptor); \
        } \
    }

RBX_DEFINE_HUMANOID_DESCRIPTION_SCALE_SETTER(BodyTypeScale, bodyTypeScale,
    humanoidDescriptionBodyTypeScale, 0.0f, 0.0f, 1.0f)
RBX_DEFINE_HUMANOID_DESCRIPTION_SCALE_SETTER(DepthScale, depthScale,
    humanoidDescriptionDepthScale, 1.0f, 0.05f, 100.0f)
RBX_DEFINE_HUMANOID_DESCRIPTION_SCALE_SETTER(HeadScale, headScale,
    humanoidDescriptionHeadScale, 1.0f, 0.05f, 100.0f)
RBX_DEFINE_HUMANOID_DESCRIPTION_SCALE_SETTER(HeightScale, heightScale,
    humanoidDescriptionHeightScale, 1.0f, 0.05f, 100.0f)
RBX_DEFINE_HUMANOID_DESCRIPTION_SCALE_SETTER(ProportionScale, proportionScale,
    humanoidDescriptionProportionScale, 0.0f, 0.0f, 1.0f)
RBX_DEFINE_HUMANOID_DESCRIPTION_SCALE_SETTER(WidthScale, widthScale,
    humanoidDescriptionWidthScale, 1.0f, 0.05f, 100.0f)

#undef RBX_DEFINE_HUMANOID_DESCRIPTION_SCALE_SETTER

AnimationConstraint::AnimationConstraint()
    : Super(new Motor6DJoint())
    , enabled(true)
    , isKinematic(false)
    , maxForce(std::numeric_limits<float>::infinity())
    , maxTorque(std::numeric_limits<float>::infinity())
    , visible(false)
    , color()
{
    setName(sAnimationConstraint);
}

PartInstance* AnimationConstraint::findOwningPart(Attachment* attachment)
{
    for (Instance* parent = attachment ? attachment->getParent() : nullptr;
         parent; parent = parent->getParent())
        if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(parent))
            return part;
    return nullptr;
}

void AnimationConstraint::configureJoint()
{
    PartInstance* first = findOwningPart(getAttachment0());
    PartInstance* second = findOwningPart(getAttachment1());
    if (!enabled || !first || !second || first == second)
    {
        setPart0(nullptr);
        setPart1(nullptr);
        return;
    }

    // Attachment frames may pass through Bones or nested Attachments. Convert
    // their fully composed world frames back into each owning part's space to
    // obtain the exact Motor6D-equivalent joint coordinates.
    setC0(first->getCoordinateFrame().toObjectSpace(getAttachment0()->getFrameInWorld()));
    setC1(second->getCoordinateFrame().toObjectSpace(getAttachment1()->getFrameInWorld()));
    setPart0(first);
    setPart1(second);
    setTransform(transform);
}

void AnimationConstraint::setAttachment0(Attachment* value)
{
    if (attachment0.lock().get() != value)
    {
        attachment0 = shared_from(value);
        raisePropertyChanged(animationAttachment0);
        configureJoint();
    }
}

void AnimationConstraint::setAttachment1(Attachment* value)
{
    if (attachment1.lock().get() != value)
    {
        attachment1 = shared_from(value);
        raisePropertyChanged(animationAttachment1);
        configureJoint();
    }
}

void AnimationConstraint::setEnabled(bool value)
{
    if (enabled != value)
    {
        enabled = value;
        raisePropertyChanged(animationEnabled);
        configureJoint();
    }
}

void AnimationConstraint::setIsKinematic(bool value)
{
    if (isKinematic != value)
    {
        isKinematic = value;
        raisePropertyChanged(animationIsKinematic);
    }
}

void AnimationConstraint::setMaxForce(float value)
{
    if (maxForce != value) { maxForce = value; raisePropertyChanged(animationMaxForce); }
}

void AnimationConstraint::setMaxTorque(float value)
{
    if (maxTorque != value) { maxTorque = value; raisePropertyChanged(animationMaxTorque); }
}

void AnimationConstraint::setVisible(bool value)
{
    if (visible != value) { visible = value; raisePropertyChanged(animationVisible); }
}

void AnimationConstraint::setColor(BrickColor value)
{
    if (color != value) { color = value; raisePropertyChanged(animationColor); }
}

void AnimationConstraint::setTransform(const CoordinateFrame& value)
{
    if (transform != value)
    {
        transform = value;
        raisePropertyChanged(animationTransform);
    }
    Vector3 axis;
    float angle = 0.0f;
    transform.rotation.toAxisAngle(axis, angle);
    static_cast<Motor6DJoint*>(getJoint())->setCurrentOffsetAngle(
        transform.translation, axis * angle);
}

const std::string& AnimationConstraint::getParentName()
{
    PartInstance* part = getPart0();
    return part ? part->getName() : IAnimatableJoint::sNULL;
}

const std::string& AnimationConstraint::getPartName()
{
    PartInstance* part = getPart1();
    return part ? part->getName() : IAnimatableJoint::sNULL;
}

void AnimationConstraint::applyPose(const CachedPose& pose)
{
    if (!enabled || !pose.initialized)
        return;
    static_cast<Motor6DJoint*>(getJoint())->applyPose(
        pose.translation, pose.rotaxisangle, pose.weight, pose.maskWeight);
    transform = pose.getCFrame();
}

void AnimationConstraint::setIsAnimatedJoint(bool value)
{
    isAnimatedJoint = value;
    if (PartInstance* part = getPart1())
        if (Assembly* assembly = part->getPartPrimitive()->getAssembly())
            assembly->setAnimationControlled(value);
}

void AnimationConstraint::onAncestorChanged(const AncestorChanged& event)
{
    Super::onAncestorChanged(event);

    // Instance::predelete detaches descendants after their last owning
    // shared_ptr has started disposal. Rebinding the joint at that point can
    // attempt shared_from() on an already-expired part. There is no joint to
    // configure once the containing hierarchy is being detached; the weak
    // endpoints and JointInstance teardown release it naturally.
    if (event.newParent)
        configureJoint();
}

BallSocketConstraint::BallSocketConstraint()
    : Super(new BallSocketJoint()), enabled(true), limitsEnabled(false), twistLimitsEnabled(false),
      upperAngle(45.0f), twistLowerAngle(-45.0f), twistUpperAngle(45.0f),
      restitution(0.0f), maxFrictionTorque(0.0f), radius(0.15f), visible(false),
      color()
{
    setName(sBallSocketConstraint);
}

#define RBX_SET_SIMPLE(member, value, descriptor) \
    do { if ((member) != (value)) { (member) = (value); raisePropertyChanged(descriptor); } } while (false)

void BallSocketConstraint::setAttachment0(Attachment* value)
{ if (attachment0.lock().get() != value) { attachment0 = shared_from(value); raisePropertyChanged(ballAttachment0); configureJoint(); } }
void BallSocketConstraint::setAttachment1(Attachment* value)
{ if (attachment1.lock().get() != value) { attachment1 = shared_from(value); raisePropertyChanged(ballAttachment1); configureJoint(); } }
void BallSocketConstraint::setEnabled(bool value) { if (enabled != value) { enabled = value; raisePropertyChanged(ballEnabled); configureJoint(); } }
void BallSocketConstraint::setLimitsEnabled(bool value) { RBX_SET_SIMPLE(limitsEnabled, value, ballLimitsEnabled); }
void BallSocketConstraint::setTwistLimitsEnabled(bool value) { RBX_SET_SIMPLE(twistLimitsEnabled, value, ballTwistLimitsEnabled); }
void BallSocketConstraint::setUpperAngle(float value) { RBX_SET_SIMPLE(upperAngle, value, ballUpperAngle); }
void BallSocketConstraint::setTwistLowerAngle(float value) { RBX_SET_SIMPLE(twistLowerAngle, value, ballTwistLowerAngle); }
void BallSocketConstraint::setTwistUpperAngle(float value) { RBX_SET_SIMPLE(twistUpperAngle, value, ballTwistUpperAngle); }
void BallSocketConstraint::setRestitution(float value) { RBX_SET_SIMPLE(restitution, value, ballRestitution); }
void BallSocketConstraint::setMaxFrictionTorque(float value) { RBX_SET_SIMPLE(maxFrictionTorque, value, ballMaxFrictionTorque); }
void BallSocketConstraint::setRadius(float value) { RBX_SET_SIMPLE(radius, value, ballRadius); }
void BallSocketConstraint::setVisible(bool value) { RBX_SET_SIMPLE(visible, value, ballVisible); }
void BallSocketConstraint::setColor(BrickColor value) { RBX_SET_SIMPLE(color, value, ballColor); }

PartInstance* BallSocketConstraint::findOwningPart(Attachment* attachment)
{
	for (Instance* parent = attachment ? attachment->getParent() : NULL;
		parent; parent = parent->getParent())
		if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(parent))
			return part;
	return NULL;
}

void BallSocketConstraint::configureJoint()
{
	PartInstance* first = findOwningPart(getAttachment0());
	PartInstance* second = findOwningPart(getAttachment1());
	if (!enabled || !first || !second || first == second)
	{
		setPart0(NULL);
		setPart1(NULL);
		return;
	}
	setC0(first->getCoordinateFrame().toObjectSpace(getAttachment0()->getFrameInWorld()));
	setC1(second->getCoordinateFrame().toObjectSpace(getAttachment1()->getFrameInWorld()));
	setPart0(first);
	setPart1(second);
}

void BallSocketConstraint::refreshJoint()
{
	configureJoint();
}

void BallSocketConstraint::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);
	if (event.newParent)
		configureJoint();
}

HingeConstraint::HingeConstraint()
    : Super(new MotorJoint())
    , enabled(true)
    , actuatorType(ACTUATOR_NONE)
    , limitsEnabled(false)
    , lowerAngle(0.0f)
    , upperAngle(45.0f)
    , targetAngle(0.0f)
    , angularSpeed(0.0f)
    , angularVelocity(0.0f)
    , angularResponsiveness(45.0f)
    , motorMaxAcceleration(500000.0f)
    , motorMaxTorque(0.0f)
    , servoMaxTorque(0.0f)
    , softlockServoUponReachingTarget(false)
    , restitution(0.0f)
    , radius(0.15f)
    , visible(false)
    , color()
{
    setName(sHingeConstraint);
}

PartInstance* HingeConstraint::findOwningPart(Attachment* attachment)
{
    for (Instance* parent = attachment ? attachment->getParent() : NULL;
         parent; parent = parent->getParent())
        if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(parent))
            return part;
    return NULL;
}

static CoordinateFrame hingeFrameInPart(PartInstance* part, Attachment* attachment)
{
    CoordinateFrame frame = part->getCoordinateFrame().toObjectSpace(
        attachment->getFrameInWorld());
    const Matrix3& rotation = frame.rotation;
    // Roblox HingeConstraint rotates around Attachment.Axis (local X), while
    // the engine revolute joint uses local Z.  A cyclic basis permutation
    // preserves handedness and maps the authored axis onto the joint axis.
    Matrix3 jointRotation;
    jointRotation.setColumn(0, rotation.column(1));
    jointRotation.setColumn(1, rotation.column(2));
    jointRotation.setColumn(2, rotation.column(0));
    frame.rotation = jointRotation;
    return frame;
}

void HingeConstraint::configureJoint()
{
    PartInstance* first = findOwningPart(getAttachment0());
    PartInstance* second = findOwningPart(getAttachment1());
    if (!enabled || !first || !second || first == second)
    {
        setPart0(NULL);
        setPart1(NULL);
        return;
    }

    setC0(hingeFrameInPart(first, getAttachment0()));
    setC1(hingeFrameInPart(second, getAttachment1()));
    setPart0(first);
    setPart1(second);
    updateActuator();
}

void HingeConstraint::updateActuator()
{
    MotorJoint* motor = static_cast<MotorJoint*>(getJoint());
    if (!enabled || actuatorType == ACTUATOR_NONE)
    {
        motor->desiredAngle = motor->getCurrentAngle();
        motor->maxVelocity = 0.0f;
        return;
    }

    float desiredDegrees = targetAngle;
    float speedDegrees = std::abs(angularSpeed);
    if (actuatorType == ACTUATOR_MOTOR)
    {
        speedDegrees = std::abs(angularVelocity);
        if (limitsEnabled)
            desiredDegrees = angularVelocity >= 0.0f ? upperAngle : lowerAngle;
        else
            desiredDegrees = angularVelocity >= 0.0f ? 1000000.0f : -1000000.0f;
    }
    else if (limitsEnabled)
    {
        desiredDegrees = std::max(lowerAngle,
            std::min(upperAngle, desiredDegrees));
    }

    motor->desiredAngle = Math::degreesToRadians(desiredDegrees);
    motor->maxVelocity = Math::degreesToRadians(speedDegrees) / 60.0f;
}

void HingeConstraint::setAttachment0(Attachment* value)
{ if (attachment0.lock().get() != value) { attachment0 = shared_from(value); raisePropertyChanged(hingeAttachment0); configureJoint(); } }
void HingeConstraint::setAttachment1(Attachment* value)
{ if (attachment1.lock().get() != value) { attachment1 = shared_from(value); raisePropertyChanged(hingeAttachment1); configureJoint(); } }
void HingeConstraint::setEnabled(bool value)
{ if (enabled != value) { enabled = value; raisePropertyChanged(hingeEnabled); configureJoint(); } }
void HingeConstraint::setActuatorType(ActuatorType value)
{ if (actuatorType != value) { actuatorType = value; raisePropertyChanged(hingeActuatorType); updateActuator(); } }
void HingeConstraint::setLimitsEnabled(bool value)
{ if (limitsEnabled != value) { limitsEnabled = value; raisePropertyChanged(hingeLimitsEnabled); updateActuator(); } }
void HingeConstraint::setLowerAngle(float value)
{ if (lowerAngle != value) { lowerAngle = value; raisePropertyChanged(hingeLowerAngle); updateActuator(); } }
void HingeConstraint::setUpperAngle(float value)
{ if (upperAngle != value) { upperAngle = value; raisePropertyChanged(hingeUpperAngle); updateActuator(); } }
void HingeConstraint::setTargetAngle(float value)
{ if (targetAngle != value) { targetAngle = value; raisePropertyChanged(hingeTargetAngle); updateActuator(); } }
void HingeConstraint::setAngularSpeed(float value)
{ if (angularSpeed != value) { angularSpeed = value; raisePropertyChanged(hingeAngularSpeed); updateActuator(); } }
void HingeConstraint::setAngularVelocity(float value)
{ if (angularVelocity != value) { angularVelocity = value; raisePropertyChanged(hingeAngularVelocity); updateActuator(); } }
void HingeConstraint::setAngularResponsiveness(float value)
{ RBX_SET_SIMPLE(angularResponsiveness, value, hingeAngularResponsiveness); }
void HingeConstraint::setMotorMaxAcceleration(float value)
{ RBX_SET_SIMPLE(motorMaxAcceleration, value, hingeMotorMaxAcceleration); }
void HingeConstraint::setMotorMaxTorque(float value)
{ RBX_SET_SIMPLE(motorMaxTorque, value, hingeMotorMaxTorque); }
void HingeConstraint::setServoMaxTorque(float value)
{ RBX_SET_SIMPLE(servoMaxTorque, value, hingeServoMaxTorque); }
void HingeConstraint::setSoftlockServoUponReachingTarget(bool value)
{ RBX_SET_SIMPLE(softlockServoUponReachingTarget, value, hingeSoftlock); }
void HingeConstraint::setRestitution(float value)
{ RBX_SET_SIMPLE(restitution, value, hingeRestitution); }
void HingeConstraint::setRadius(float value)
{ RBX_SET_SIMPLE(radius, value, hingeRadius); }
void HingeConstraint::setVisible(bool value)
{ RBX_SET_SIMPLE(visible, value, hingeVisible); }
void HingeConstraint::setColor(BrickColor value)
{ RBX_SET_SIMPLE(color, value, hingeColor); }

float HingeConstraint::getCurrentAngle() const
{
    return Math::radiansToDegrees(
        static_cast<const MotorJoint*>(
            const_cast<HingeConstraint*>(this)->getJoint())->getCurrentAngle());
}

void HingeConstraint::refreshJoint()
{
    configureJoint();
}

void HingeConstraint::onAncestorChanged(const AncestorChanged& event)
{
    Super::onAncestorChanged(event);
    if (event.newParent)
        configureJoint();
}

NoCollisionConstraint::NoCollisionConstraint() : enabled(true)
{ setName(sNoCollisionConstraint); }

NoCollisionConstraint::~NoCollisionConstraint()
{
	clearCollisionPair();
}

void NoCollisionConstraint::clearCollisionPair()
{
	shared_ptr<PartInstance> first = activePart0.lock();
	shared_ptr<PartInstance> second = activePart1.lock();
	if (first && second)
		first->getPartPrimitive()->setCanCollideWith(
			*second->getPartPrimitive(), true);
	activePart0.reset();
	activePart1.reset();
}

void NoCollisionConstraint::updateCollisionPair()
{
	clearCollisionPair();
	shared_ptr<PartInstance> first = part0.lock();
	shared_ptr<PartInstance> second = part1.lock();
	if (!enabled || !first || !second || first == second ||
		!Workspace::getWorkspaceIfInWorkspace(this) ||
		Workspace::getWorkspaceIfInWorkspace(first.get()) !=
			Workspace::getWorkspaceIfInWorkspace(second.get()))
		return;
	first->getPartPrimitive()->setCanCollideWith(
		*second->getPartPrimitive(), false);
	activePart0 = first;
	activePart1 = second;
}

void NoCollisionConstraint::setPart0(PartInstance* value)
{ if (part0.lock().get() != value) { part0 = shared_from(value); raisePropertyChanged(noCollisionPart0); updateCollisionPair(); } }
void NoCollisionConstraint::setPart1(PartInstance* value)
{ if (part1.lock().get() != value) { part1 = shared_from(value); raisePropertyChanged(noCollisionPart1); updateCollisionPair(); } }
void NoCollisionConstraint::setEnabled(bool value)
{ if (enabled != value) { enabled = value; raisePropertyChanged(noCollisionEnabled); updateCollisionPair(); } }

void NoCollisionConstraint::onAncestorChanged(const AncestorChanged& event)
{
	Instance::onAncestorChanged(event);
	if (event.newParent)
		updateCollisionPair();
	else
		clearCollisionPair();
}

WrapTarget::WrapTarget() : stiffness(0.0f) { setName(sWrapTarget); }
void WrapTarget::setCageMeshId(const MeshId& value) { RBX_SET_SIMPLE(cageMeshId, value, wrapCageMeshId); }
void WrapTarget::setTemporaryCageMeshId(const MeshId& value) { RBX_SET_SIMPLE(temporaryCageMeshId, value, wrapTemporaryCageMeshId); }
void WrapTarget::setHSRAssetId(const ContentId& value) { RBX_SET_SIMPLE(hsrAssetId, value, wrapHsrAssetId); }
void WrapTarget::setHSRData(const BinaryString& value) { RBX_SET_SIMPLE(hsrData, value, wrapHsrData); }
void WrapTarget::setHSRMeshId(const MeshId& value) { RBX_SET_SIMPLE(hsrMeshId, value, wrapHsrMeshId); }
void WrapTarget::setCageOrigin(const CoordinateFrame& value) { RBX_SET_SIMPLE(cageOrigin, value, wrapCageOrigin); }
void WrapTarget::setImportOrigin(const CoordinateFrame& value) { RBX_SET_SIMPLE(importOrigin, value, wrapImportOrigin); }
void WrapTarget::setStiffness(float value) { RBX_SET_SIMPLE(stiffness, value, wrapStiffness); }
void WrapTarget::verifySetParent(const Instance* parent) const
{
    Instance::verifySetParent(parent);
    if (parent && !Instance::fastDynamicCast<const MeshPart>(parent))
        throw RBX::runtime_error("WrapTarget can only be parented to a MeshPart");
}

FaceControls::FaceControls() { setName(sFaceControls); }
void FaceControls::setInternalOverrideFACSData(const std::string& value)
{ RBX_SET_SIMPLE(overrideFacsData, value, faceOverrideFacsData); }

#undef RBX_SET_SIMPLE

} // namespace RBX
