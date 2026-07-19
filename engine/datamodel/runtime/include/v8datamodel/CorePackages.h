#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

#include <string>

namespace RBX {

extern const char* const sCorePackages;

class CorePackages
    : public DescribedCreatable<CorePackages, Instance, sCorePackages>
    , public Service
{
public:
    CorePackages();

    void setPatchAssetManifest(std::string manifest);
    const std::string& getPatchAssetManifest() const { return patchAssetManifest; }

private:
    std::string patchAssetManifest;
};

} // namespace RBX
