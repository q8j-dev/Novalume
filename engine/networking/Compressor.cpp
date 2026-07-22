/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */

#include "Compressor.h"
#include "Replicator.h"
#include "util/Quaternion.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/copy.hpp>

using namespace RBX;
using namespace RBX::Network;

enum
{
	translationBits0 = 15,
	translationBits1 = 14,
	translationBits2 = 15
};

static const float translationMin[] = { -1024, -512, -1024 };
static const float translationMax[] = { 1024, 512, 1024 };

void Compressor::writeRotation(RBX::Network::PacketBuffer& bitStream, const Matrix3& rotation, CompressionType compressionType)
{
	writeCompressionType(bitStream, compressionType);

	Quaternion q(rotation);
	q.normalize();

	switch (compressionType)
	{
	case Compressor::UNCOMPRESSED:
		{
			bitStream << q.w;
			bitStream << q.x;
			bitStream << q.y;
			bitStream << q.z;
			break;
		}
	case Compressor::COMPRESSED:
		{
			bitStream.WriteNormQuat(q.w, q.x, q.y, q.z);
			break;
		}
	case Compressor::HEAVILY_COMPRESSED:
		{
			bitStream.WriteNormQuat(q.w, q.x, q.y, q.z);
            break;
		}
	default:
		{
			RBXASSERT(0);
		}
	}
}


void Compressor::writeTranslation(RBX::Network::PacketBuffer& bitStream, const Vector3& translation, CompressionType compressionType)
{
	if ((compressionType == Compressor::HEAVILY_COMPRESSED) && !canHeavilyCompressTranslation(translation)) {
		compressionType = Compressor::COMPRESSED;
	}

	writeCompressionType(bitStream, compressionType);

	switch (compressionType)
	{
	case Compressor::UNCOMPRESSED:
		{
			bitStream << translation.x;
			bitStream << translation.y;
			bitStream << translation.z;
			break;
		}
	case Compressor::COMPRESSED:
		{
			bitStream.WriteVector(translation.x, translation.y, translation.z);
			break;
		}
	case Compressor::HEAVILY_COMPRESSED:
		{
			// p = (p_i - min)*shift/(max-min)
			unsigned short x = (unsigned short)((translation.x - translationMin[0])*(1 << translationBits0)/(translationMax[0]-translationMin[0]));
			unsigned short y = (unsigned short)((translation.y - translationMin[1])*(1 << translationBits1)/(translationMax[1]-translationMin[1]));
			unsigned short z = (unsigned short)((translation.z - translationMin[2])*(1 << translationBits2)/(translationMax[2]-translationMin[2]));

			// Check for overflow (underflow shouldn't be possible)
			if (x >= 1 << translationBits0) x = 0xFFFF;
			if (y >= 1 << translationBits1) y = 0xFFFF;
			if (z >= 1 << translationBits2) z = 0xFFFF;

			bitStream.WriteBits((const unsigned char*) &x, translationBits0);
			bitStream.WriteBits((const unsigned char*) &y, translationBits1);
			bitStream.WriteBits((const unsigned char*) &z, translationBits2);
			break;
		}
	default:
		{
			RBXASSERT(0);
		}
	}
}

bool Compressor::canHeavilyCompressTranslation(const Vector3& translation)
{
	return !(	translation.x>translationMax[0] || translation.x<translationMin[0] ||
				translation.y>translationMax[1] || translation.y<translationMin[1] ||
				translation.z>translationMax[2] || translation.z<translationMin[2]	);
}

/////////////////////////////////////////////////////////////////////////////////////


void Compressor::readRotation(RBX::Network::PacketBuffer& bitStream, Matrix3& rotation)
{
	CompressionType compressionType = readCompressionType(bitStream);

	Quaternion q;

	switch (compressionType)
	{
	case UNCOMPRESSED:
		{
			bitStream >> q.w;
			bitStream >> q.x;
			bitStream >> q.y;
			bitStream >> q.z;
			break;
		}
	case COMPRESSED:
        {
            if (!bitStream.ReadNormQuat(q.w, q.x, q.y, q.z))
                throw std::runtime_error("Failed to read Quaternion");
            break;
        }
	case HEAVILY_COMPRESSED:
		{
            if (!bitStream.ReadNormQuat(q.w, q.x, q.y, q.z))
                throw std::runtime_error("Failed to read Quaternion");
            break;
		}
	default:
		{
			RBXASSERT(0);
		}
	}

	q.toRotationMatrix(rotation);
}

void Compressor::readTranslation(RBX::Network::PacketBuffer& bitStream, Vector3& translation)
{

	CompressionType compressionType = readCompressionType(bitStream);

	switch (compressionType)
	{
	case UNCOMPRESSED:
		{
			bitStream >> translation.x;
			bitStream >> translation.y;
			bitStream >> translation.z;
			break;
		}
	case COMPRESSED:
		{
			readVectorFast( bitStream, translation.x, translation.y, translation.z );
			break;
		}
	case HEAVILY_COMPRESSED:
		{
			unsigned short x,y,z = 0;

			readFastN<translationBits0>( bitStream, x );
			readFastN<translationBits1>( bitStream, y );
			readFastN<translationBits2>( bitStream, z );

			// p_i = p/shift * (max-min) + min
			translation.x = (float)x * (1.0f/(1 << translationBits0) * (translationMax[0]-translationMin[0])) + translationMin[0];
			translation.y = (float)y * (1.0f/(1 << translationBits1) * (translationMax[1]-translationMin[1])) + translationMin[1];
			translation.z = (float)z * (1.0f/(1 << translationBits2) * (translationMax[2]-translationMin[2])) + translationMin[2];
			break;
		}
	default:
		{
			RBXASSERT(0);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

void Compressor::writeCompressionType(RBX::Network::PacketBuffer& bitStream, CompressionType compressionType)
{
	unsigned short temp = compressionType;
	bitStream.WriteBits((const unsigned char*) &temp, 2);
}

Compressor::CompressionType Compressor::readCompressionType(RBX::Network::PacketBuffer& bitStream)
{
	unsigned short temp = 0;
	readFastN<2>( bitStream, temp );

	CompressionType answer = static_cast<CompressionType>(temp);
	return answer;
}

