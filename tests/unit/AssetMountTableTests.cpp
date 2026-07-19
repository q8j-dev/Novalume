#include "rbx/assets/AssetMountTable.h"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

void write(const std::filesystem::path& path, std::string_view value)
{
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path, std::ios::binary);
    stream.write(value.data(), static_cast<std::streamsize>(value.size()));
}

} // namespace

int main()
{
    namespace fs = std::filesystem;
    using RBX::Assets::AssetMountTable;

    const fs::path root = fs::temp_directory_path() /
        ("rbx-asset-mount-test-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()));
    const fs::path legacy = root / "legacy";
    const fs::path overlay = root / "player-overlay";
    const fs::path platform = root / "platform";
    write(legacy / "textures/ui/Chat/Chat.png", "legacy");
    write(overlay / "textures/ui/Chat/Chat.png", "player");
    write(overlay / "textures/ui/Chat/Chat@2x.png", "player-2x");
    write(overlay / "textures/ui/Chat/Chat@3x.png", "player-3x");
    write(platform / "textures/ui/Controls/Touch.png", "platform");
    write(overlay / "assets/123456789.png", "embedded-id");

    AssetMountTable table;
    table.addMount("legacy", legacy, 0);
    table.addMount("player-2026", overlay, 100);
    table.addMount("platform", platform, 200);

    const auto chat = table.resolve("rbxasset://textures/ui/Chat/Chat.png");
    require(chat && chat->mountName == "player-2026", "overlay must supersede the legacy mount");
    require(chat->path == fs::canonical(overlay / "textures/ui/Chat/Chat.png"),
        "overlay resolved the wrong file");

    const auto retina = table.resolve("rbxasset://textures/ui/Chat/Chat.png", 2.0f);
    require(retina && retina->logicalPath.ends_with("Chat@2x.png"), "2x density variant was not selected");
    require(retina && retina->densityScale == 2, "2x density scale was not reported");
    const auto dense = table.resolve("rbxasset://textures/ui/Chat/Chat.png", 3.0f);
    require(dense && dense->logicalPath.ends_with("Chat@3x.png"), "3x density variant was not selected");

    const auto touch = table.resolve("rbxasset://textures/ui/Controls/Touch.png");
    require(touch && touch->mountName == "platform", "platform mount must take highest precedence");
    const auto fallback = table.resolve("rbxasset://textures/ui/Chat/Chat.png", 1.25f);
    require(fallback && fallback->logicalPath.ends_with("Chat.png"), "base-density fallback is wrong");

    require(!table.resolve("rbxasset://../outside.png"), "parent traversal must be rejected");
    require(!table.resolve("rbxasset:///absolute.png"), "absolute asset path must be rejected");
    require(!table.resolve("https://example.invalid/asset.png"), "non-asset URL must be rejected");
    require(!table.resolve("rbxasset://textures/ui/Chat/Chat.png", 0), "invalid density must be rejected");
    const auto embedded = table.resolveAssetId("123456789");
    require(embedded && embedded->path ==
        fs::canonical(overlay / "assets/123456789.png"),
        "numeric embedded asset did not resolve from a mounted assets directory");
    require(!table.resolveAssetId("../123456789"),
        "invalid numeric embedded asset ID must be rejected");

    bool duplicateRejected = false;
    try
    {
        table.addMount("legacy", legacy, 500);
    }
    catch (const std::invalid_argument&)
    {
        duplicateRejected = true;
    }
    require(duplicateRejected, "duplicate mount name must be rejected");

    fs::remove_all(root);
    return 0;
}
