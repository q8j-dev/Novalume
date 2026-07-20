#pragma once

#include <filesystem>
#include <vector>

namespace rbx::platform {

[[nodiscard]] std::vector<std::filesystem::path> loadRecentDocuments(
    const std::filesystem::path& writableRoot);
void recordRecentDocument(const std::filesystem::path& writableRoot,
                          const std::filesystem::path& document);

} // namespace rbx::platform
