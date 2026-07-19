#pragma once

#include <filesystem>

namespace RBX::Assets {

struct MaterializedPlacePackage
{
    std::filesystem::path root;
    std::filesystem::path place;
};

[[nodiscard]] bool isPlacePackage(const std::filesystem::path& path);

// Validates and materializes an RBXLP package into a deterministic local cache.
// The returned root contains the original place plus an assets/<id>.<format>
// directory suitable for mounting through AssetMountTable.
[[nodiscard]] MaterializedPlacePackage materializePlacePackage(
    const std::filesystem::path& package,
    const std::filesystem::path& cacheRoot);

} // namespace RBX::Assets
