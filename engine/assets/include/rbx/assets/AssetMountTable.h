#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <vector>

namespace RBX::Assets {

struct AssetMount
{
    std::string name;
    std::filesystem::path root;
    int priority = 0;
};

struct ResolvedAsset
{
    std::filesystem::path path;
    std::string mountName;
    std::string logicalPath;
    float densityScale = 1;
};

class AssetMountTable final
{
public:
    void addMount(std::string name, const std::filesystem::path& root, int priority);
    void clear();

    [[nodiscard]] std::optional<ResolvedAsset> resolve(
        std::string_view logicalPath, float pixelDensity = 1) const;
    [[nodiscard]] std::optional<ResolvedAsset> resolveAssetId(
        std::string_view assetId) const;
    [[nodiscard]] std::vector<AssetMount> mounts() const;

private:
    struct Entry
    {
        AssetMount mount;
        std::uint64_t sequence = 0;
    };

    mutable std::shared_mutex mutex;
    std::vector<Entry> entries;
    std::uint64_t nextSequence = 0;
};

// Shared by ContentProvider and the player bootstrap so every rbxasset lookup
// observes the same ordered mounts.
AssetMountTable& assetMountTable();

} // namespace RBX::Assets
