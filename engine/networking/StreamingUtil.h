//
//  StreamingUtil.h
//  Network
//
//  Created by Martin Robaszewski on 5/2/13.
//  Copyright (c) 2013 Roblox Inc. All rights reserved.
//

#ifndef Network_StreamingUtil_h
#define Network_StreamingUtil_h

#include "network/PacketBuffer.h"

namespace RBX
{
	class BrickColor;
	class UDim;
	class UDim2;
	class Faces;
	class Axes;
	class SystemAddress;
    class BinaryString;
    class NumberSequence;
    class ColorSequence;
    class NumberRange;
    class NumberSequenceKeypoint;
    class ColorSequenceKeypoint;
	class PhysicalProperties;
	class Font;

	namespace StreamRegion {
		class Id;
	}
    
    
	template<class T>
	RBX::Network::PacketBuffer& operator >> (RBX::Network::PacketBuffer& stream, T& value);
    
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, char value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, signed char value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, unsigned char value);

    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, short value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, unsigned short value);
    
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, int value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, unsigned int value);

    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, unsigned long long value);

    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, bool value);

	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, float value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, double value);

    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const RBX::Guid::Scope& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const BinaryString& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const std::string& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const ContentId& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const StreamRegion::Id& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const G3D::Vector3& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const G3D::Vector2& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const G3D::Vector3int16& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const G3D::Vector2int16& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const G3D::Color3& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const G3D::CoordinateFrame& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const RBX::Velocity& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, RBX::SystemAddress value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const BrickColor& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const UDim& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const UDim2& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const RBX::RbxRay& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const Faces& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const Axes& value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const NumberSequence& value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const ColorSequence& value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const NumberRange& value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const NumberSequenceKeypoint& value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const ColorSequenceKeypoint& value);
	RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const Rect2D& value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const PhysicalProperties& value);
    RBX::Network::PacketBuffer& operator << (RBX::Network::PacketBuffer& stream, const Font& value);
    RBX::Network::PacketBuffer& operator >> (RBX::Network::PacketBuffer& stream, Font& value);
        
}

#endif
