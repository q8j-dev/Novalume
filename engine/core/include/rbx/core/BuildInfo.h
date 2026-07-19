#pragma once

#include <string_view>

namespace rbx::core {

struct BuildInfo final {
    static constexpr std::string_view productName = "RobloxPlayer";
    static constexpr std::string_view architecture = "shared-cross-platform";
};

} // namespace rbx::core
