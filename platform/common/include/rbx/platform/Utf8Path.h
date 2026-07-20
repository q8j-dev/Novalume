#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace rbx::platform {

// std::filesystem::path(const char*) uses the active ANSI code page on
// Windows. Player-facing strings are UTF-8 on every host, so all crossings
// between script/argv text and native paths must use the UTF-8 path APIs.
inline std::filesystem::path pathFromUtf8(std::string_view value)
{
    std::u8string encoded;
    encoded.reserve(value.size());
    for (const char byte : value)
        encoded.push_back(
            static_cast<char8_t>(static_cast<unsigned char>(byte)));
    return std::filesystem::path(encoded);
}

inline std::filesystem::path pathFromUtf8(std::u8string_view value)
{
    return std::filesystem::path(std::u8string(value));
}

inline std::string pathToUtf8(const std::filesystem::path& value)
{
    const std::u8string encoded = value.u8string();
    return std::string(reinterpret_cast<const char*>(encoded.data()),
                       encoded.size());
}

} // namespace rbx::platform
