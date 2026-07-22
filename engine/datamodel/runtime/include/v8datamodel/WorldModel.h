#pragma once

#include "v8datamodel/WorldRoot.h"
#include "util/SteppedInstance.h"

namespace RBX {

extern const char* const sWorldModel;

class WorldModel
    : public DescribedCreatable<WorldModel, WorldRoot, sWorldModel>
    , public IStepped
{
private:
    typedef DescribedCreatable<WorldModel, WorldRoot, sWorldModel> Super;
    double simulationTime;

protected:
    /*override*/ bool askSetParent(const Instance* instance) const;
    /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
    /*override*/ void onStepped(const Stepped& event);

public:
    WorldModel();

    /*override*/ Camera* getCamera() { return NULL; }
    /*override*/ const Camera* getConstCamera() const { return NULL; }
    /*override*/ const ModelInstance* getCameraOwnerModel() const { return this; }
};

} // namespace RBX
