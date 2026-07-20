#pragma once

#include "V8DataModel/Attachment.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/PartInstance.h"
#include "V8Tree/Instance.h"
#include "Util/MeshId.h"
#include "Util/BinaryString.h"

namespace RBX
{

extern const char* const sAnimationConstraint;
extern const char* const sBallSocketConstraint;
extern const char* const sHingeConstraint;
extern const char* const sNoCollisionConstraint;
extern const char* const sWrapTarget;
extern const char* const sFaceControls;
extern const char* const sHumanoidDescription;

class HumanoidDescription final
    : public DescribedCreatable<HumanoidDescription, Instance,
        sHumanoidDescription>
{
public:
    typedef DescribedCreatable<HumanoidDescription, Instance,
        sHumanoidDescription> Super;

    HumanoidDescription();

    const std::string& getBackAccessory() const { return backAccessory; }
    void setBackAccessory(const std::string& value);
    const std::string& getFaceAccessory() const { return faceAccessory; }
    void setFaceAccessory(const std::string& value);
    const std::string& getFrontAccessory() const { return frontAccessory; }
    void setFrontAccessory(const std::string& value);
    const std::string& getHairAccessory() const { return hairAccessory; }
    void setHairAccessory(const std::string& value);
    const std::string& getHatAccessory() const { return hatAccessory; }
    void setHatAccessory(const std::string& value);
    const std::string& getNeckAccessory() const { return neckAccessory; }
    void setNeckAccessory(const std::string& value);
    const std::string& getShouldersAccessory() const { return shouldersAccessory; }
    void setShouldersAccessory(const std::string& value);
    const std::string& getWaistAccessory() const { return waistAccessory; }
    void setWaistAccessory(const std::string& value);

    float getBodyTypeScale() const { return bodyTypeScale; }
    void setBodyTypeScale(float value);
    float getDepthScale() const { return depthScale; }
    void setDepthScale(float value);
    float getHeadScale() const { return headScale; }
    void setHeadScale(float value);
    float getHeightScale() const { return heightScale; }
    void setHeightScale(float value);
    float getProportionScale() const { return proportionScale; }
    void setProportionScale(float value);
    float getWidthScale() const { return widthScale; }
    void setWidthScale(float value);

private:
    std::string backAccessory;
    std::string faceAccessory;
    std::string frontAccessory;
    std::string hairAccessory;
    std::string hatAccessory;
    std::string neckAccessory;
    std::string shouldersAccessory;
    std::string waistAccessory;
    float bodyTypeScale;
    float depthScale;
    float headScale;
    float heightScale;
    float proportionScale;
    float widthScale;
};

class AnimationConstraint final
    : public DescribedCreatable<AnimationConstraint, JointInstance, sAnimationConstraint>
    , public IAnimatableJoint
{
public:
    typedef DescribedCreatable<AnimationConstraint, JointInstance,
        sAnimationConstraint> Super;
    AnimationConstraint();

    Attachment* getAttachment0() const { return attachment0.lock().get(); }
    Attachment* getAttachment1() const { return attachment1.lock().get(); }
    void setAttachment0(Attachment* value);
    void setAttachment1(Attachment* value);

    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);
    bool getIsKinematic() const { return isKinematic; }
    void setIsKinematic(bool value);
    float getMaxForce() const { return maxForce; }
    void setMaxForce(float value);
    float getMaxTorque() const { return maxTorque; }
    void setMaxTorque(float value);
    bool getVisible() const { return visible; }
    void setVisible(bool value);
    BrickColor getColor() const { return color; }
    void setColor(BrickColor value);
    const CoordinateFrame& getTransform() const { return transform; }
    void setTransform(const CoordinateFrame& value);
	void refreshJoint() { configureJoint(); }

    const std::string& getParentName() override;
    const std::string& getPartName() override;
    void applyPose(const CachedPose& pose) override;
    void setIsAnimatedJoint(bool value) override;

protected:
    void onAncestorChanged(const AncestorChanged& event) override;

private:
    void configureJoint();
    static PartInstance* findOwningPart(Attachment* attachment);

    weak_ptr<Attachment> attachment0;
    weak_ptr<Attachment> attachment1;
    bool enabled;
    bool isKinematic;
    float maxForce;
    float maxTorque;
    bool visible;
    BrickColor color;
    CoordinateFrame transform;
};

class BallSocketConstraint final
    : public DescribedCreatable<BallSocketConstraint, JointInstance, sBallSocketConstraint>
{
public:
	typedef DescribedCreatable<BallSocketConstraint, JointInstance,
		sBallSocketConstraint> Super;
    BallSocketConstraint();
    Attachment* getAttachment0() const { return attachment0.lock().get(); }
    Attachment* getAttachment1() const { return attachment1.lock().get(); }
    void setAttachment0(Attachment* value);
    void setAttachment1(Attachment* value);

    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);
    bool getLimitsEnabled() const { return limitsEnabled; }
    void setLimitsEnabled(bool value);
    bool getTwistLimitsEnabled() const { return twistLimitsEnabled; }
    void setTwistLimitsEnabled(bool value);
    float getUpperAngle() const { return upperAngle; }
    void setUpperAngle(float value);
    float getTwistLowerAngle() const { return twistLowerAngle; }
    void setTwistLowerAngle(float value);
    float getTwistUpperAngle() const { return twistUpperAngle; }
    void setTwistUpperAngle(float value);
    float getRestitution() const { return restitution; }
    void setRestitution(float value);
    float getMaxFrictionTorque() const { return maxFrictionTorque; }
    void setMaxFrictionTorque(float value);
    float getRadius() const { return radius; }
    void setRadius(float value);
    bool getVisible() const { return visible; }
    void setVisible(bool value);
    BrickColor getColor() const { return color; }
    void setColor(BrickColor value);
	void refreshJoint();

protected:
	void onAncestorChanged(const AncestorChanged& event) override;

private:
	static PartInstance* findOwningPart(Attachment* attachment);
	void configureJoint();
    weak_ptr<Attachment> attachment0;
    weak_ptr<Attachment> attachment1;
    bool enabled;
    bool limitsEnabled;
    bool twistLimitsEnabled;
    float upperAngle;
    float twistLowerAngle;
    float twistUpperAngle;
    float restitution;
    float maxFrictionTorque;
    float radius;
    bool visible;
    BrickColor color;
};

class HingeConstraint final
    : public DescribedCreatable<HingeConstraint, JointInstance, sHingeConstraint>
{
public:
    typedef DescribedCreatable<HingeConstraint, JointInstance,
        sHingeConstraint> Super;

    enum ActuatorType
    {
        ACTUATOR_NONE = 0,
        ACTUATOR_MOTOR = 1,
        ACTUATOR_SERVO = 2
    };

    HingeConstraint();
    Attachment* getAttachment0() const { return attachment0.lock().get(); }
    Attachment* getAttachment1() const { return attachment1.lock().get(); }
    void setAttachment0(Attachment* value);
    void setAttachment1(Attachment* value);
    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);
    ActuatorType getActuatorType() const { return actuatorType; }
    void setActuatorType(ActuatorType value);
    bool getLimitsEnabled() const { return limitsEnabled; }
    void setLimitsEnabled(bool value);
    float getLowerAngle() const { return lowerAngle; }
    void setLowerAngle(float value);
    float getUpperAngle() const { return upperAngle; }
    void setUpperAngle(float value);
    float getTargetAngle() const { return targetAngle; }
    void setTargetAngle(float value);
    float getAngularSpeed() const { return angularSpeed; }
    void setAngularSpeed(float value);
    float getAngularVelocity() const { return angularVelocity; }
    void setAngularVelocity(float value);
    float getAngularResponsiveness() const { return angularResponsiveness; }
    void setAngularResponsiveness(float value);
    float getMotorMaxAcceleration() const { return motorMaxAcceleration; }
    void setMotorMaxAcceleration(float value);
    float getMotorMaxTorque() const { return motorMaxTorque; }
    void setMotorMaxTorque(float value);
    float getServoMaxTorque() const { return servoMaxTorque; }
    void setServoMaxTorque(float value);
    bool getSoftlockServoUponReachingTarget() const { return softlockServoUponReachingTarget; }
    void setSoftlockServoUponReachingTarget(bool value);
    float getRestitution() const { return restitution; }
    void setRestitution(float value);
    float getRadius() const { return radius; }
    void setRadius(float value);
    bool getVisible() const { return visible; }
    void setVisible(bool value);
    BrickColor getColor() const { return color; }
    void setColor(BrickColor value);
    float getCurrentAngle() const;
    void refreshJoint();

protected:
    void onAncestorChanged(const AncestorChanged& event) override;

private:
    static PartInstance* findOwningPart(Attachment* attachment);
    void configureJoint();
    void updateActuator();

    weak_ptr<Attachment> attachment0;
    weak_ptr<Attachment> attachment1;
    bool enabled;
    ActuatorType actuatorType;
    bool limitsEnabled;
    float lowerAngle;
    float upperAngle;
    float targetAngle;
    float angularSpeed;
    float angularVelocity;
    float angularResponsiveness;
    float motorMaxAcceleration;
    float motorMaxTorque;
    float servoMaxTorque;
    bool softlockServoUponReachingTarget;
    float restitution;
    float radius;
    bool visible;
    BrickColor color;
};

class NoCollisionConstraint final
    : public DescribedCreatable<NoCollisionConstraint, Instance, sNoCollisionConstraint>
{
public:
    NoCollisionConstraint();
	~NoCollisionConstraint() override;
    PartInstance* getPart0() const { return part0.lock().get(); }
    PartInstance* getPart1() const { return part1.lock().get(); }
    void setPart0(PartInstance* value);
    void setPart1(PartInstance* value);
    bool getEnabled() const { return enabled; }
    void setEnabled(bool value);

private:
	void updateCollisionPair();
	void clearCollisionPair();
	void onAncestorChanged(const AncestorChanged& event) override;

    weak_ptr<PartInstance> part0;
    weak_ptr<PartInstance> part1;
	weak_ptr<PartInstance> activePart0;
	weak_ptr<PartInstance> activePart1;
    bool enabled;
};

class WrapTarget final
    : public DescribedCreatable<WrapTarget, Instance, sWrapTarget>
{
public:
    WrapTarget();
    const MeshId& getCageMeshId() const { return cageMeshId; }
    void setCageMeshId(const MeshId& value);
    const MeshId& getTemporaryCageMeshId() const { return temporaryCageMeshId; }
    void setTemporaryCageMeshId(const MeshId& value);
    const ContentId& getHSRAssetId() const { return hsrAssetId; }
    void setHSRAssetId(const ContentId& value);
	const BinaryString& getHSRData() const { return hsrData; }
	void setHSRData(const BinaryString& value);
	const MeshId& getHSRMeshId() const { return hsrMeshId; }
	void setHSRMeshId(const MeshId& value);
    const CoordinateFrame& getCageOrigin() const { return cageOrigin; }
    void setCageOrigin(const CoordinateFrame& value);
    const CoordinateFrame& getImportOrigin() const { return importOrigin; }
    void setImportOrigin(const CoordinateFrame& value);
    float getStiffness() const { return stiffness; }
    void setStiffness(float value);

protected:
    void verifySetParent(const Instance* parent) const override;

private:
    MeshId cageMeshId;
    MeshId temporaryCageMeshId;
    ContentId hsrAssetId;
	BinaryString hsrData;
	MeshId hsrMeshId;
    CoordinateFrame cageOrigin;
    CoordinateFrame importOrigin;
    float stiffness;
};

class FaceControls final
    : public DescribedCreatable<FaceControls, Instance, sFaceControls>
{
public:
    FaceControls();
    const std::string& getInternalOverrideFACSData() const { return overrideFacsData; }
    void setInternalOverrideFACSData(const std::string& value);

private:
    std::string overrideFacsData;
};

} // namespace RBX

namespace RBX { namespace Reflection {
template<> EnumDesc<RBX::HingeConstraint::ActuatorType>::EnumDesc();
} }
