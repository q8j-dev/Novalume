#pragma once

namespace RBX::Security::ProtectionMarkers
{
// Historical builds used proprietary mutation/virtualization markers around
// selected routines. The portable runtime deliberately treats these markers
// as semantic no-ops; platform security belongs at owned process and package
// boundaries instead of inside gameplay code.
inline void beginMutation() noexcept {}
inline void beginVirtualization() noexcept {}
inline void end() noexcept {}
}
