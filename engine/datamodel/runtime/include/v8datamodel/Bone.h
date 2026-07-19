#pragma once

#include "V8DataModel/Attachment.h"
#include "V8DataModel/IAnimatableJoint.h"

namespace RBX
{

extern const char* const sBone;

class Bone final
    : public DescribedCreatable<Bone, Attachment, sBone>
    , public IAnimatableJoint
{
public:
    Bone();

    const CoordinateFrame& getTransform() const { return transform; }
    void setTransform(const CoordinateFrame& value);

    CoordinateFrame getTransformedCFrame() const;
    CoordinateFrame getTransformedWorldCFrame() const;
    CoordinateFrame getParentFrame() const override;

    const std::string& getParentName() override;
    const std::string& getPartName() override;
    void applyPose(const CachedPose& pose) override;

protected:
    void verifySetParent(const Instance* instance) const override;

private:
    CoordinateFrame transform;
};

} // namespace RBX
