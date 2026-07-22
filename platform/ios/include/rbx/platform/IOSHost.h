#pragma once

#include "rbx/platform/Host.h"

#include <memory>

namespace rbx::platform {

[[nodiscard]] std::unique_ptr<Host> createIOSHost(std::uint32_t width,
                                                  std::uint32_t height,
                                                  bool visible);

}
