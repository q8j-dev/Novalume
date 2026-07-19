#pragma once

#include <cstddef>
#include <filesystem>

namespace RBX {

class DataModel;

class DataModelPatch
{
public:
    struct Result
    {
        std::size_t coreScriptCount = 0;
        std::size_t dataModelInstanceCount = 0;
        std::size_t assetManifestBytes = 0;
    };

    static Result applyBundled(DataModel* dataModel,
        const std::filesystem::path& modelPath,
        const std::filesystem::path& checksumPath);
};

} // namespace RBX
