#include "Util/FileSystem.h"

#include <cstdlib>
#include <filesystem>
#include <string>
#include <vector>

#include <unistd.h>

namespace RBX::FileSystem {
namespace {

boost::filesystem::path homeDirectory()
{
    if (const char* home = std::getenv("HOME"))
        return home;
    return {};
}

boost::filesystem::path executableDirectory()
{
    std::vector<char> buffer(4096);
    for (;;) {
        const ssize_t size = readlink("/proc/self/exe", buffer.data(), buffer.size());
        if (size < 0)
            return {};
        if (static_cast<std::size_t>(size) < buffer.size())
            return boost::filesystem::path(std::string(buffer.data(), size)).parent_path();
        buffer.resize(buffer.size() * 2);
    }
}

boost::filesystem::path appendAndCreate(boost::filesystem::path path, bool create,
                                        const char* subDirectory)
{
    path /= "Roblox";
    if (subDirectory && *subDirectory)
        path /= subDirectory;
    boost::system::error_code error;
    if (create)
        boost::filesystem::create_directories(path, error);
    return error ? boost::filesystem::path{} : path;
}

} // namespace

boost::filesystem::path getUserDirectory(bool create, FileSystemDir directory,
                                         const char* subDirectory)
{
    if (directory == DirExe) {
        auto path = executableDirectory();
        if (subDirectory && *subDirectory)
            path /= subDirectory;
        return path;
    }

    auto home = homeDirectory();
    switch (directory) {
    case DirAppData:
        if (const char* dataHome = std::getenv("XDG_DATA_HOME"))
            return appendAndCreate(dataHome, create, subDirectory);
        return appendAndCreate(home / ".local" / "share", create, subDirectory);
    case DirPicture:
        return appendAndCreate(home / "Pictures", create, subDirectory);
    case DirVideo:
        return appendAndCreate(home / "Videos", create, subDirectory);
    default:
        return {};
    }
}

boost::filesystem::path getLogsDirectory()
{
    if (const char* stateHome = std::getenv("XDG_STATE_HOME"))
        return appendAndCreate(stateHome, true, "logs");
    return appendAndCreate(homeDirectory() / ".local" / "state", true, "logs");
}

} // namespace RBX::FileSystem
