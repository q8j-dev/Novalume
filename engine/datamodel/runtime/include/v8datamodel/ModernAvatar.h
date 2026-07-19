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
extern const char* const sNoCollisionConstraint;
extern const char* const sWrapTarget;
extern const char* const sFaceControls;

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
