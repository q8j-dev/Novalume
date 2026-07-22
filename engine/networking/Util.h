#pragma once

#include "util/SystemAddress.h"
#include "util/G3DCore.h"
#include "network/NetworkTypes.h"

#ifdef _WIN32
#if defined(_NOOPT) || defined(_DEBUG) || defined(RBX_TEST_BUILD)
#define NETWORK_PROFILER
#define NETWORK_DEBUG
#endif
#endif

namespace RBX {

namespace SpatialRegion {
	class Id;
}

namespace Network {

	const RBX::SystemAddress peerAddressToSystemAddress(const PeerAddress& address);
	std::string peerAddressToString(const PeerAddress& address, bool writePort = true, char portDelimiter='|');

}}	// namespace
