#include "rbx/core/BuildInfo.h"

static_assert(!rbx::core::BuildInfo::productName.empty());
static_assert(rbx::core::BuildInfo::versionMajor >= 0);
static_assert(rbx::core::BuildInfo::versionMinor >= 0);
static_assert(rbx::core::BuildInfo::versionPatch >= 0);
static_assert(rbx::core::BuildInfo::versionRevision >= 0);
static_assert(!rbx::core::BuildInfo::architecture.empty());
