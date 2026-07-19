#pragma once

#include "MeshFileStructs.h"

#include "util/Object.h"
#include "util/G3DCore.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace RBX
{
	struct FileMeshData
	{
		struct VertexExtra
		{
			std::array<std::int8_t, 4> tangent = {0, 0, 0, 0};
			std::array<std::uint8_t, 4> color = {255, 255, 255, 255};
		};

		struct Skinning
		{
			std::array<std::uint8_t, 4> boneIndices = {0, 0, 0, 0};
			std::array<std::uint8_t, 4> boneWeights = {255, 0, 0, 0};
		};

		struct Bone
		{
			std::string name;
			std::uint16_t parentIndex = 0xffff;
			std::uint16_t lodParentIndex = 0xffff;
			float culling = 0.0f;
			CoordinateFrame bindFrame;
		};

		struct Subset
		{
			std::uint32_t facesBegin = 0;
			std::uint32_t facesLength = 0;
			std::uint32_t vertsBegin = 0;
			std::uint32_t vertsLength = 0;
			std::vector<std::uint16_t> boneIndices;
		};

		std::vector<FileMeshVertexNormalTexture3d> vnts;
		std::vector<VertexExtra> vertexExtras;
		std::vector<FileMeshFace> faces;
		std::vector<std::uint32_t> lodOffsets;
		std::vector<Skinning> skinning;
		std::vector<Bone> bones;
		std::vector<Subset> subsets;
		AABox aabb;
	};

    shared_ptr<FileMeshData> ReadFileMesh(const std::string& data);
	
	// writes the newest version always.
	// remember: set ostream to binary!
	void WriteFileMesh(std::ostream& f, const FileMeshData& mesh);
}
