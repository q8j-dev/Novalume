#include "Util.h"
#include "v8world/ContactManagerSpatialHash.h"

#include <boost/static_assert.hpp>

namespace RBX { 
namespace Network {

	const RBX::SystemAddress peerAddressToSystemAddress(const PeerAddress& address)
	{
		return RBX::SystemAddress(address.GetBinaryAddress(), address.GetPort());
	}

	std::string peerAddressToString(const PeerAddress& address, bool writePort, char portDelimiter)
	{
		return address.ToString(writePort, portDelimiter);
	}

}}	// namespace
