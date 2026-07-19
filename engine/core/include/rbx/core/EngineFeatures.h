#pragma once

#include <string_view>

namespace RBX::EngineFeatures {

// Engine features describe native capabilities that CoreScripts may probe.
// A feature is enabled only after its backing engine behavior is implemented.
void registerFeature(std::string_view name);
bool isEnabled(std::string_view name);

} // namespace RBX::EngineFeatures
