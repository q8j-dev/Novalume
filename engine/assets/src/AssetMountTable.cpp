#include "rbx/assets/AssetMountTable.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <stdexcept>

namespace RBX::Assets {
namespace {

constexpr std::string_view assetScheme = "rbxasset://";

bool isInside(const std::filesystem::path& candidate, const std::filesystem::path& root)
{
    auto candidatePart = candidate.begin();
    auto rootPart = root.begin();
    for (; rootPart != root.end(); ++rootPart, ++candidatePart)
    {
        if (candidatePart == candidate.end() || *candidatePart != *rootPart)
            return false;
    }
    return true;
}

std::optional<std::filesystem::path> validatedRelativePath(std::string_view logicalPath)
{
    if (!logicalPath.starts_with(assetScheme) || logicalPath.size() == assetScheme.size())
        return std::nullopt;

    const std::string_view suffix = logicalPath.substr(assetScheme.size());
    if (suffix.find('\\') != std::string_view::npos || suffix.find('\0') != std::string_view::npos)
        return std::nullopt;
    for (const char value : suffix)
    {
        const auto character = static_cast<unsigned char>(value);
        if (character < 0x20 || character == 0x7f)
            return std::nullopt;
    }

    const std::filesystem::path relative{std::string(suffix)};
    if (relative.empty() || relative.is_absolute() || relative.has_root_name() || relative.has_root_directory())
        return std::nullopt;
    for (const std::filesystem::path& part : relative)
    {
        if (part == "." || part == ".." || part.empty())
            return std::nullopt;
    }
    return relative;
}

std::vector<std::pair<std::filesystem::path, float>> densityCandidates(
    const std::filesystem::path& relative, float pixelDensity)
{
    std::vector<std::pair<std::filesystem::path, float>> result;
    const std::string stem = relative.stem().string();
    const std::string extension = relative.extension().string();
    const bool explicitDensity = stem.ends_with("@2x") || stem.ends_with("@3x");
    if (!explicitDensity && !extension.empty())
    {
        if (pixelDensity >= 2.5f)
            result.emplace_back(relative.parent_path() / (stem + "@3x" + extension), 3);
        if (pixelDensity >= 1.5f)
            result.emplace_back(relative.parent_path() / (stem + "@2x" + extension), 2);
    }
    result.emplace_back(relative, explicitDensity && stem.ends_with("@3x") ? 3 :
        explicitDensity && stem.ends_with("@2x") ? 2 : 1);
    return result;
}

} // namespace

void AssetMountTable::addMount(std::string name, const std::filesystem::path& root, int priority)
{
    if (name.empty())
        throw std::invalid_argument("asset mount name must not be empty");
    std::error_code error;
    const std::filesystem::path canonicalRoot = std::filesystem::canonical(root, error);
    if (error || !std::filesystem::is_directory(canonicalRoot, error) || error)
        throw std::invalid_argument("asset mount root must be an existing directory");

    std::unique_lock lock(mutex);
    if (std::ranges::any_of(entries, [&](const Entry& entry) { return entry.mount.name == name; }))
        throw std::invalid_argument("asset mount names must be unique");
    entries.push_back({{std::move(name), canonicalRoot, priority}, nextSequence++});
    std::ranges::stable_sort(entries, [](const Entry& left, const Entry& right) {
        if (left.mount.priority != right.mount.priority)
            return left.mount.priority > right.mount.priority;
        return left.sequence > right.sequence;
    });
}

void AssetMountTable::clear()
{
    std::unique_lock lock(mutex);
    entries.clear();
    nextSequence = 0;
}

std::optional<ResolvedAsset> AssetMountTable::resolve(
    std::string_view logicalPath, float pixelDensity) const
{
    if (!std::isfinite(pixelDensity) || pixelDensity <= 0)
        return std::nullopt;
    const std::optional<std::filesystem::path> relative = validatedRelativePath(logicalPath);
    if (!relative)
        return std::nullopt;

    std::shared_lock lock(mutex);
    for (const Entry& entry : entries)
    {
        for (const auto& [candidateRelative, density] : densityCandidates(*relative, pixelDensity))
        {
            std::error_code error;
            const std::filesystem::path unresolved = entry.mount.root / candidateRelative;
            if (!std::filesystem::is_regular_file(unresolved, error) || error)
                continue;
            const std::filesystem::path candidate = std::filesystem::canonical(unresolved, error);
            if (error || !isInside(candidate, entry.mount.root))
                continue;
            return ResolvedAsset{
                .path = candidate,
                .mountName = entry.mount.name,
                .logicalPath = std::string(assetScheme) + candidateRelative.generic_string(),
                .densityScale = density,
            };
        }
    }
    return std::nullopt;
}

std::optional<ResolvedAsset> AssetMountTable::resolveAssetId(
    std::string_view assetId) const
{
    if (assetId.empty() || !std::ranges::all_of(assetId,
            [](char value) { return value >= '0' && value <= '9'; }))
        return std::nullopt;

    std::shared_lock lock(mutex);
    for (const Entry& entry : entries)
    {
        const std::filesystem::path assetsRoot = entry.mount.root / "assets";
        std::error_code error;
        if (!std::filesystem::is_directory(assetsRoot, error) || error)
            continue;

        std::vector<std::filesystem::path> candidates;
        for (std::filesystem::directory_iterator iterator(assetsRoot, error), end;
             !error && iterator != end; iterator.increment(error))
        {
            if (!iterator->is_regular_file(error) || error)
                continue;
            if (iterator->path().stem() == assetId)
                candidates.push_back(iterator->path());
        }
        if (error || candidates.empty())
            continue;
        std::ranges::sort(candidates);
        const std::filesystem::path candidate =
            std::filesystem::canonical(candidates.front(), error);
        const std::filesystem::path canonicalAssetsRoot =
            std::filesystem::canonical(assetsRoot, error);
        if (error || !isInside(candidate, canonicalAssetsRoot))
            continue;
        return ResolvedAsset{
            .path = candidate,
            .mountName = entry.mount.name,
            .logicalPath = "rbxassetid://" + std::string(assetId),
            .densityScale = 1,
        };
    }
    return std::nullopt;
}

std::vector<AssetMount> AssetMountTable::mounts() const
{
    std::shared_lock lock(mutex);
    std::vector<AssetMount> result;
    result.reserve(entries.size());
    for (const Entry& entry : entries)
        result.push_back(entry.mount);
    return result;
}

AssetMountTable& assetMountTable()
{
    static AssetMountTable table;
    return table;
}

} // namespace RBX::Assets
