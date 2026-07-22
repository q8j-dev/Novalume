#pragma once

#include "v8datamodel/RootInstance.h"

namespace RBX {

extern const char* const sWorldRoot;

class WorldRoot : public Reflection::Described<WorldRoot, sWorldRoot, RootInstance>
{
protected:
    WorldRoot();

public:
    static WorldRoot* findWorldRoot(Instance* context);
    static const WorldRoot* findConstWorldRoot(const Instance* context);
    static World* getWorldIfInWorldRoot(Instance* context);
};

} // namespace RBX
