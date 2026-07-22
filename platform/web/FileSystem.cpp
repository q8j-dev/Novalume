#include "util/FileSystem.h"

#include <boost/filesystem.hpp>

namespace RBX::FileSystem {
namespace {

boost::filesystem::path appendAndCreate(boost::filesystem::path path, bool create,
    const char* subDirectory)
{
    if (subDirectory && *subDirectory)
        path /= subDirectory;
    boost::system::error_code error;
    if (create)
        boost::filesystem::create_directories(path, error);
    return error ? boost::filesystem::path{} : path;
}

}

boost::filesystem::path getUserDirectory(bool create, FileSystemDir directory,
    const char* subDirectory)
{
    switch (directory) {
    case DirExe:
        return appendAndCreate("/Resources", false, subDirectory);
    case DirPicture:
        return appendAndCreate("/persistent/Roblox/Pictures", create, subDirectory);
    case DirVideo:
        return appendAndCreate("/persistent/Roblox/Videos", create, subDirectory);
    case DirAppData:
        return appendAndCreate("/persistent/Roblox", create, subDirectory);
    default:
        return {};
    }
}

boost::filesystem::path getLogsDirectory()
{
    return appendAndCreate("/persistent/Roblox", true, "logs");
}

}
