#include "rbx/core/EngineFeatures.h"

#include <mutex>
#include <string>
#include <unordered_set>

namespace RBX::EngineFeatures {
namespace {
std::mutex featureMutex;
std::unordered_set<std::string> features;
}

void registerFeature(std::string_view name)
{
    std::lock_guard<std::mutex> lock(featureMutex);
    features.emplace(name);
}

bool isEnabled(std::string_view name)
{
    std::lock_guard<std::mutex> lock(featureMutex);
    return features.find(std::string(name)) != features.end();
}

} // namespace RBX::EngineFeatures
