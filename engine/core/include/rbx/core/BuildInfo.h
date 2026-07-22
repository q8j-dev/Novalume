#pragma once

#include <string_view>

#if !defined(RBX_PRODUCT_VERSION_MAJOR) || \
    !defined(RBX_PRODUCT_VERSION_MINOR) || \
    !defined(RBX_PRODUCT_VERSION_PATCH) || \
    !defined(RBX_PRODUCT_VERSION_REVISION)
#error "Novalume version metadata must be supplied by the rbx-core target"
#endif

namespace rbx::core {

struct BuildInfo final {
    static constexpr std::string_view productName = "RobloxPlayer";
    static constexpr int versionMajor = RBX_PRODUCT_VERSION_MAJOR;
    static constexpr int versionMinor = RBX_PRODUCT_VERSION_MINOR;
    static constexpr int versionPatch = RBX_PRODUCT_VERSION_PATCH;
    static constexpr int versionRevision = RBX_PRODUCT_VERSION_REVISION;

#if defined(__aarch64__) || defined(__arm64__) || defined(_M_ARM64)
    static constexpr std::string_view architecture = "arm64";
#elif defined(__x86_64__) || defined(_M_X64)
    static constexpr std::string_view architecture = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
    static constexpr std::string_view architecture = "x86";
#elif defined(__arm__) || defined(_M_ARM)
    static constexpr std::string_view architecture = "arm";
#elif defined(__wasm64__)
    static constexpr std::string_view architecture = "wasm64";
#elif defined(__wasm32__)
    static constexpr std::string_view architecture = "wasm32";
#else
    static constexpr std::string_view architecture = "unknown";
#endif
};

} // namespace rbx::core
