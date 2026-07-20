#include "rbx/platform/RecentDocuments.h"
#include "rbx/platform/Utf8Path.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <string>
#include <system_error>

namespace rbx::platform {
namespace {

constexpr std::size_t kMaximumRecentDocuments = 8;

std::filesystem::path databasePath(const std::filesystem::path& writableRoot)
{
    return writableRoot / "recent-documents.v1";
}

void saveRecentDocuments(const std::filesystem::path& writableRoot,
                         const std::vector<std::filesystem::path>& documents)
{
    std::error_code error;
    std::filesystem::create_directories(writableRoot, error);
    if (error)
        return;
    const auto destination = databasePath(writableRoot);
    auto temporary = destination;
    temporary += ".tmp";
    {
        std::ofstream output(temporary, std::ios::trunc);
        if (!output)
            return;
        output << "RBXREC1\n";
        for (const auto& document : documents)
            output << std::quoted(pathToUtf8(document)) << '\n';
        if (!output)
            return;
    }
    std::filesystem::rename(temporary, destination, error);
    if (!error)
        return;
    error.clear();
    std::filesystem::remove(destination, error);
    error.clear();
    std::filesystem::rename(temporary, destination, error);
}

} // namespace

std::vector<std::filesystem::path> loadRecentDocuments(
    const std::filesystem::path& writableRoot)
{
    std::ifstream input(databasePath(writableRoot));
    std::string magic;
    if (!std::getline(input, magic) || magic != "RBXREC1")
        return {};
    std::vector<std::filesystem::path> result;
    std::string value;
    while (result.size() < kMaximumRecentDocuments && input >> std::quoted(value)) {
        std::filesystem::path document;
        try {
            document = pathFromUtf8(value);
        } catch (const std::filesystem::filesystem_error&) {
            continue;
        }
        std::error_code error;
        if (!std::filesystem::is_regular_file(document, error) || error)
            continue;
        if (std::find(result.begin(), result.end(), document) == result.end())
            result.push_back(std::move(document));
    }
    return result;
}

void recordRecentDocument(const std::filesystem::path& writableRoot,
                          const std::filesystem::path& document)
{
    std::error_code error;
    if (!std::filesystem::is_regular_file(document, error) || error)
        return;
    auto documents = loadRecentDocuments(writableRoot);
    documents.erase(std::remove(documents.begin(), documents.end(), document),
        documents.end());
    documents.insert(documents.begin(), document);
    if (documents.size() > kMaximumRecentDocuments)
        documents.resize(kMaximumRecentDocuments);
    saveRecentDocuments(writableRoot, documents);
}

} // namespace rbx::platform
