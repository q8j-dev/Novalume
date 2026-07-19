#pragma once

#include "network/PacketBuffer.h"

#include "Util/G3DCore.h"


namespace RBX { 

namespace Network {

	class Compressor
	{
	public:
		typedef enum {UNCOMPRESSED = 0, COMPRESSED, HEAVILY_COMPRESSED} CompressionType;

	private:
		static bool canHeavilyCompressTranslation(const Vector3& translation);

		static void writeCompressionType(RBX::Network::PacketBuffer& bitStream, CompressionType compressionType);
		static CompressionType readCompressionType(RBX::Network::PacketBuffer& bitStream);

	public:
		static void writeTranslation(RBX::Network::PacketBuffer& bitStream, const Vector3& translation, CompressionType compressionType);
		static void writeRotation(RBX::Network::PacketBuffer& bitStream, const Matrix3& rotation, CompressionType compressionType);

		static void readTranslation(RBX::Network::PacketBuffer& bitStream, Vector3& translation);
		static void readRotation(RBX::Network::PacketBuffer& bitStream, Matrix3& rotation);
	};

}}