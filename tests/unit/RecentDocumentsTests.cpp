#include "rbx/platform/RecentDocuments.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
        throw std::runtime_error(message);
}

class TemporaryDirectory final {
public:
    TemporaryDirectory()
    {
        const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
            ("novalume-recents-" + std::to_string(nonce));
        std::filesystem::create_directories(path);
    }

    ~TemporaryDirectory()
    {
        std::error_code error;
        std::filesystem::remove_all(path, error);
    }

    std::filesystem::path path;
};

void createDocument(const std::filesystem::path& path)
{
    std::ofstream output(path, std::ios::binary);
    output << "<roblox/>";
    require(static_cast<bool>(output), "could not create recent-document fixture");
}

} // namespace

int main()
try {
    TemporaryDirectory temporary;
    const auto data = temporary.path / "data";
    const auto first = temporary.path / "First Place.rbxlx";
    const auto second = temporary.path / "Quoted \"Place\".rbxlx";
    createDocument(first);
    createDocument(second);

    rbx::platform::recordRecentDocument(data, first);
    rbx::platform::recordRecentDocument(data, second);
    rbx::platform::recordRecentDocument(data, first);
    auto documents = rbx::platform::loadRecentDocuments(data);
    require(documents.size() == 2, "recent documents were not deduplicated");
    require(documents[0] == first && documents[1] == second,
        "recent documents are not in most-recent-first order");

    std::filesystem::remove(first);
    documents = rbx::platform::loadRecentDocuments(data);
    require(documents.size() == 1 && documents[0] == second,
        "missing recent documents were not pruned during load");

    std::cout << "recent-document persistence contract passed\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
