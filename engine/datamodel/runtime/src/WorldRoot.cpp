#include "V8DataModel/WorldRoot.h"

namespace RBX {

const char* const sWorldRoot = "WorldRoot";

WorldRoot::WorldRoot()
{
}

WorldRoot* WorldRoot::findWorldRoot(Instance* context)
{
    for (Instance* current = context; current; current = current->getParent())
        if (WorldRoot* root = Instance::fastDynamicCast<WorldRoot>(current))
            return root;
    return NULL;
}

const WorldRoot* WorldRoot::findConstWorldRoot(const Instance* context)
{
    return findWorldRoot(const_cast<Instance*>(context));
}

World* WorldRoot::getWorldIfInWorldRoot(Instance* context)
{
    WorldRoot* root = findWorldRoot(context);
    return root && (root == context || context->isDescendantOf(root)) ? root->getWorld() : NULL;
}

} // namespace RBX
