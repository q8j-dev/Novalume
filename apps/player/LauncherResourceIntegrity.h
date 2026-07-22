#pragma once

#include <filesystem>

namespace rbx::player
{

void verifyLauncherResourceIntegrity(const std::filesystem::path& resourceRoot);

}
