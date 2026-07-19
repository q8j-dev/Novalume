#include "V8DataModel/CorePackages.h"

namespace RBX {

const char* const sCorePackages = "CorePackages";

CorePackages::CorePackages()
    : Service(true)
{
    setName(sCorePackages);
    setRobloxLocked(true);
}

void CorePackages::setPatchAssetManifest(std::string manifest)
{
    patchAssetManifest = std::move(manifest);
}

} // namespace RBX
