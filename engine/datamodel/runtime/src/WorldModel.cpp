#include "v8datamodel/WorldModel.h"

#include "v8world/World.h"

namespace RBX {

const char* const sWorldModel = "WorldModel";

WorldModel::WorldModel()
    : simulationTime(0.0)
{
    setName(sWorldModel);
}

bool WorldModel::askSetParent(const Instance* instance) const
{
    return instance != NULL;
}

void WorldModel::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
    Super::onServiceProvider(oldProvider, newProvider);
    onServiceProviderIStepped(oldProvider, newProvider);
}

void WorldModel::onStepped(const Stepped& event)
{
    if (event.gameStep <= 0)
        return;

    simulationTime += event.gameStep;
    getWorld()->assemble();
    getWorld()->step(false, simulationTime, static_cast<float>(event.gameStep), 1);
}

} // namespace RBX
