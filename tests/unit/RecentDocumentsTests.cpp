#include "rbx/platform/RecentDocuments.h"
#include "rbx/platform/Utf8Path.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
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
    const auto unicode = temporary.path /
        rbx::platform::pathFromUtf8(u8"\u65e5\u672c\u8a9e \U0001f9ed Place.rbxlx");
    createDocument(first);
    createDocument(second);
    createDocument(unicode);

    rbx::platform::recordRecentDocument(data, first);
    rbx::platform::recordRecentDocument(data, second);
    rbx::platform::recordRecentDocument(data, unicode);
    rbx::platform::recordRecentDocument(data, first);
    auto documents = rbx::platform::loadRecentDocuments(data);
    require(documents.size() == 3, "recent documents were not deduplicated");
    require(documents[0] == first && documents[1] == unicode &&
            documents[2] == second,
        "recent documents are not in most-recent-first order");
    require(rbx::platform::pathFromUtf8(
                rbx::platform::pathToUtf8(documents[1])) == unicode,
        "recent document UTF-8 path conversion did not round-trip");
    std::ifstream database(data / "recent-documents.v1", std::ios::binary);
    const std::string serialized{
        std::istreambuf_iterator<char>(database), std::istreambuf_iterator<char>()};
    require(serialized.find(rbx::platform::pathToUtf8(unicode)) !=
            std::string::npos,
        "recent document database did not persist the Unicode path as UTF-8");

    std::filesystem::remove(first);
    documents = rbx::platform::loadRecentDocuments(data);
    require(documents.size() == 2 && documents[0] == unicode &&
            documents[1] == second,
        "missing recent documents were not pruned during load");

    std::cout << "recent-document persistence contract passed\n";
    return 0;
} catch (const std::exception& error) {
    std::cerr << error.what() << '\n';
    return 1;
}
