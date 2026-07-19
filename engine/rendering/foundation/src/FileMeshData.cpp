#include "GfxBase/FileMeshData.h"

#include "rbx/Debug.h"

#include "rbx/DenseHash.h"

#include "draco/attributes/point_attribute.h"
#include "draco/compression/decode.h"
#include "draco/core/decoder_buffer.h"
#include "draco/mesh/mesh.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <set>
#include <utility>

namespace RBX
{
    struct MeshVertexHasher
    {
        bool operator()(const FileMeshVertexNormalTexture3d& l, const FileMeshVertexNormalTexture3d& r) const
        {
            return memcmp(&l, &r, sizeof(l)) == 0;
        }
        
        size_t operator()(const FileMeshVertexNormalTexture3d& v) const
        {
            size_t result = 0;
            boost::hash_combine(result, v.vx);
            boost::hash_combine(result, v.vy);
            boost::hash_combine(result, v.vz);
            return result;
        }
    };
    
    void optimizeMesh(FileMeshData& mesh)
    {
        std::vector<unsigned int> remap(mesh.vnts.size());
        
        FileMeshVertexNormalTexture3d dummy = {};
        dummy.vx = FLT_MAX;
        
        typedef DenseHashMap<FileMeshVertexNormalTexture3d, unsigned int, MeshVertexHasher, MeshVertexHasher> VertexMap;
        VertexMap vertexMap(dummy);
        
        for (size_t i = 0; i < mesh.vnts.size(); ++i)
        {
            unsigned int& vi = vertexMap[mesh.vnts[i]];
            
            if (vi == 0)
                vi = vertexMap.size();
            
            remap[i] = vi - 1;
        }
        
        std::vector<FileMeshVertexNormalTexture3d> newvnts(vertexMap.size());
        
        for (size_t i = 0; i < mesh.vnts.size(); ++i)
            newvnts[remap[i]] = mesh.vnts[i];
        
        mesh.vnts.swap(newvnts);
        
        for (size_t i = 0; i < mesh.faces.size(); ++i)
        {
            FileMeshFace& face = mesh.faces[i];
            
            face.a = remap[face.a];
            face.b = remap[face.b];
            face.c = remap[face.c];
        }
    }
    
    inline unsigned int atouFast(const char* value, const char** end)
    {
        const char* s = value;
        
        // skip whitespace
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
            s++;
        
        // read integer part
        unsigned int result = 0;
        
        while (static_cast<unsigned int>(*s - '0') < 10)
        {
            result = result * 10 + (*s - '0');
            s++;
        }
        
        // done!
        *end = s;
        
        return result;
    }

    inline double atofFast(const char* value, const char** end)
    {
        static const double digits[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
        static const double powers[] = { 1e0, 1e+1, 1e+2, 1e+3, 1e+4, 1e+5, 1e+6, 1e+7, 1e+8, 1e+9, 1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19, 1e+20, 1e+21, 1e+22 };
        
        const char* s = value;
        
        // skip whitespace
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n')
            s++;
        
        // read sign
        double sign = (*s == '-') ? -1 : 1;
        s += (*s == '-' || *s == '+');
        
        // read integer part
        double result = 0;
        int power = 0;
        
        while (static_cast<unsigned int>(*s - '0') < 10)
        {
            result = result * 10 + digits[*s - '0'];
            s++;
        }
        
        // read fractional part
        if (*s == '.')
        {
            s++;
            
            while (static_cast<unsigned int>(*s - '0') < 10)
            {
                result = result * 10 + digits[*s - '0'];
                s++;
                power--;
            }
        }
        
        // read exponent part
        if ((*s | ' ') == 'e')
        {
            s++;
            
            // read exponent sign
            int expsign = (*s == '-') ? -1 : 1;
            s += (*s == '-' || *s == '+');
            
            // read exponent
            int exppower = 0;
            
            while (static_cast<unsigned int>(*s - '0') < 10)
            {
                exppower = exppower * 10 + (*s - '0');
                s++;
            }
            
            // done!
            power += expsign * exppower;
        }
        
        // done!
        *end = s;
        
        if (static_cast<unsigned int>(-power) < sizeof(powers) / sizeof(powers[0]))
            return sign * result / powers[-power];
        else if (static_cast<unsigned int>(power) < sizeof(powers) / sizeof(powers[0]))
            return sign * result * powers[power];
        else
            return sign * result * powf(10.0, power);
    }
    
    inline const char* readToken(const char* data, char terminator)
    {
        while (*data == ' ' || *data == '\t' || *data == '\r' || *data == '\n')
            ++data;
        
        if (*data != terminator)
            throw RBX::runtime_error("Error reading mesh data: expected %c", terminator);
        
        return data + 1;
    }
    
    inline const char* readFloatToken(const char* data, char terminator, float* output)
    {
        const char* end;
        double value = atofFast(data, &end);
        
        if (*end != terminator)
            throw RBX::runtime_error("Error reading mesh data: expected %c", terminator);

        *output = value;
        
        return end + 1;
    }
    
    shared_ptr<FileMeshData> readMeshFromV1(const std::string& data, size_t offset_, float scaler)
    {
        shared_ptr<FileMeshData> mesh(new FileMeshData());
        
        const char* offset = data.c_str() + offset_;
        unsigned int num_faces = atouFast(offset, &offset);
        
        mesh->vnts.reserve(num_faces * 3);
        mesh->faces.reserve(num_faces);
        
        for (unsigned int i = 0; i < num_faces; i++)
        {
            for (int v = 0; v < 3; v++)
            {
                float vx, vy, vz, nx, ny, nz, tu, tv, tw;
                
                offset = readToken(offset, '[');
                offset = readFloatToken(offset, ',', &vx);
                offset = readFloatToken(offset, ',', &vy);
                offset = readFloatToken(offset, ']', &vz);
                offset = readToken(offset, '[');
                offset = readFloatToken(offset, ',', &nx);
                offset = readFloatToken(offset, ',', &ny);
                offset = readFloatToken(offset, ']', &nz);
                offset = readToken(offset, '[');
                offset = readFloatToken(offset, ',', &tu);
                offset = readFloatToken(offset, ',', &tv);
                offset = readFloatToken(offset, ']', &tw);
                
                G3D::Vector3 normal = G3D::Vector3(nx, ny, nz).unit();
                
                if (!normal.isFinite())
                    normal = G3D::Vector3::zero();
                
                FileMeshVertexNormalTexture3d vtx =
                {
                    vx * scaler, vy * scaler, vz * scaler,
                    normal.x, normal.y, normal.z,
                    tu, 1.f - tv, tw
                };
                
                mesh->vnts.push_back(vtx);
            }
            
            FileMeshFace face = {i * 3 + 0, i * 3 + 1, i * 3 + 2};
            
            mesh->faces.push_back(face);
        }
        
        optimizeMesh(*mesh);
        
        return mesh;
    }

    static void readData(const std::string& data, size_t& offset, void* buffer, size_t size)
    {
        if (offset + size > data.size())
            throw RBX::runtime_error("Error reading mesh data: offset is out of bounds while reading %d bytes", (int)size);

        memcpy(buffer, data.data() + offset, size);
        offset += size;
    }

    constexpr std::size_t kModernVertexSize = 40;
    constexpr std::size_t kModernVertexPrefixSize = 36;

    static std::uint16_t readU16(const std::string& data, std::size_t& offset)
    {
        std::uint16_t value;
        readData(data, offset, &value, sizeof(value));
        return value;
    }

    static std::uint32_t readU32(const std::string& data, std::size_t& offset)
    {
        std::uint32_t value;
        readData(data, offset, &value, sizeof(value));
        return value;
    }

    static float readFloat(const std::string& data, std::size_t& offset)
    {
        float value;
        readData(data, offset, &value, sizeof(value));
        return value;
    }

    static void checkCountFits(const std::string& data, std::size_t offset,
        std::size_t count, std::size_t stride, const char* what)
    {
        if (stride == 0 || count > (data.size() - std::min(offset, data.size())) / stride)
            throw std::runtime_error(std::string("Error reading mesh data: invalid ") + what + " count");
    }

    static void readVertices(const std::string& data, std::size_t& offset,
        std::size_t count, std::size_t stride, FileMeshData& mesh)
    {
        // Position, normal, UV, and tangent occupy the first 36 bytes.  The
        // optional four-byte vertex color was appended later.
        if (stride < kModernVertexPrefixSize)
            throw std::runtime_error("Error reading mesh data: incompatible vertex stride");
        checkCountFits(data, offset, count, stride, "vertex");

        mesh.vnts.resize(count);
        mesh.vertexExtras.resize(count);
        for (std::size_t index = 0; index < count; ++index)
        {
            const std::size_t record = offset;
            FileMeshVertexNormalTexture3d& vertex = mesh.vnts[index];
            readData(data, offset, &vertex.vx, sizeof(float) * 8);
            vertex.tw = 0.0f;
            readData(data, offset, mesh.vertexExtras[index].tangent.data(), 4);
            if (stride >= kModernVertexSize)
                readData(data, offset, mesh.vertexExtras[index].color.data(), 4);
            offset = record + stride;
        }
    }

    static void readFaces(const std::string& data, std::size_t& offset,
        std::size_t count, std::size_t stride, FileMeshData& mesh)
    {
        if (stride < sizeof(FileMeshFace))
            throw std::runtime_error("Error reading mesh data: incompatible face stride");
        checkCountFits(data, offset, count, stride, "face");
        mesh.faces.resize(count);
        for (FileMeshFace& face : mesh.faces)
        {
            const std::size_t record = offset;
            readData(data, offset, &face, sizeof(face));
            offset = record + stride;
        }
    }

    static void validateFaces(const FileMeshData& mesh)
    {
        if (mesh.vnts.empty() || mesh.faces.empty())
            throw std::runtime_error("Error reading mesh data: empty mesh");
        for (const FileMeshFace& face : mesh.faces)
            if (face.a >= mesh.vnts.size() || face.b >= mesh.vnts.size() ||
                face.c >= mesh.vnts.size())
                throw std::runtime_error("Error reading mesh data: index value out of range");
    }

    static shared_ptr<FileMeshData> readMeshFromV2(const std::string& data, size_t offset)
    {
        shared_ptr<FileMeshData> mesh(new FileMeshData());
        FileMeshHeader header;
        readData(data, offset, &header, sizeof(header));
        if (header.cbSize < sizeof(FileMeshHeader))
            throw std::runtime_error("Error reading mesh data: incompatible header stride");
        offset += header.cbSize - sizeof(FileMeshHeader);
        if (offset > data.size())
            throw std::runtime_error("Error reading mesh data: header is out of bounds");

        readVertices(data, offset, header.num_vertices, header.cbVerticesStride, *mesh);
        readFaces(data, offset, header.num_faces, header.cbFaceStride, *mesh);
        if (offset != data.size())
            throw std::runtime_error("Error reading mesh data: unexpected data at end of file");
        validateFaces(*mesh);
        return mesh;
    }

    static shared_ptr<FileMeshData> readMeshFromV3(const std::string& data, size_t offset)
    {
        shared_ptr<FileMeshData> mesh(new FileMeshData());
        const std::uint16_t headerSize = readU16(data, offset);
        std::uint8_t vertexStride = 0;
        std::uint8_t faceStride = 0;
        readData(data, offset, &vertexStride, 1);
        readData(data, offset, &faceStride, 1);
        const std::uint16_t lodStride = readU16(data, offset);
        const std::uint16_t lodCount = readU16(data, offset);
        const std::uint32_t vertexCount = readU32(data, offset);
        const std::uint32_t faceCount = readU32(data, offset);
        if (headerSize < 16 || lodStride < sizeof(std::uint32_t))
            throw std::runtime_error("Error reading mesh data: incompatible v3 header");
        offset += headerSize - 16;
        readVertices(data, offset, vertexCount, vertexStride, *mesh);
        readFaces(data, offset, faceCount, faceStride, *mesh);
        checkCountFits(data, offset, lodCount, lodStride, "LOD");
        mesh->lodOffsets.resize(lodCount);
        for (std::uint32_t& lod : mesh->lodOffsets)
        {
            const std::size_t record = offset;
            lod = readU32(data, offset);
            offset = record + lodStride;
        }
        if (offset != data.size())
            throw std::runtime_error("Error reading mesh data: unexpected data at end of file");
        validateFaces(*mesh);
        return mesh;
    }

    struct RawSkinning
    {
        std::array<std::uint8_t, 4> subsetIndices;
        std::array<std::uint8_t, 4> weights;
    };

    struct RawSubset
    {
        std::uint32_t facesBegin;
        std::uint32_t facesLength;
        std::uint32_t vertsBegin;
        std::uint32_t vertsLength;
        std::uint32_t numBoneIndices;
        std::array<std::uint16_t, 26> boneIndices;
    };

    static void readSkinningPayload(const std::string& data, std::size_t& offset,
        std::size_t end, FileMeshData& mesh)
    {
        const std::uint32_t skinCount = readU32(data, offset);
        if (skinCount != mesh.vnts.size())
            throw std::runtime_error("Error reading mesh data: skinning count does not match vertices");
        checkCountFits(data, offset, skinCount, 8, "skinning");
        std::vector<RawSkinning> rawSkinning(skinCount);
        for (RawSkinning& skin : rawSkinning)
        {
            readData(data, offset, skin.subsetIndices.data(), 4);
            readData(data, offset, skin.weights.data(), 4);
        }

        const std::uint32_t boneCount = readU32(data, offset);
		if (boneCount > 256)
			throw std::runtime_error("Error reading mesh data: too many skinning bones");
        checkCountFits(data, offset, boneCount, 60, "bone");
        mesh.bones.resize(boneCount);
        std::vector<std::uint32_t> boneNameOffsets(boneCount);
        for (std::uint32_t index = 0; index < boneCount; ++index)
        {
            FileMeshData::Bone& bone = mesh.bones[index];
            boneNameOffsets[index] = readU32(data, offset);
            bone.parentIndex = readU16(data, offset);
            bone.lodParentIndex = readU16(data, offset);
            bone.culling = readFloat(data, offset);
            float rotation[9];
            readData(data, offset, rotation, sizeof(rotation));
            const Vector3 translation(
                readFloat(data, offset), readFloat(data, offset), readFloat(data, offset));
            bone.bindFrame = CoordinateFrame(Matrix3(
                rotation[0], rotation[1], rotation[2],
                rotation[3], rotation[4], rotation[5],
                rotation[6], rotation[7], rotation[8]), translation);
            if (bone.parentIndex != 0xffff && bone.parentIndex >= boneCount)
                throw std::runtime_error("Error reading mesh data: invalid bone parent");
        }

        const std::uint32_t nameTableSize = readU32(data, offset);
        if (offset + nameTableSize > end)
            throw std::runtime_error("Error reading mesh data: bone name table is out of bounds");
        const std::string_view names(data.data() + offset, nameTableSize);
        offset += nameTableSize;
        for (std::size_t index = 0; index < mesh.bones.size(); ++index)
        {
            const std::uint32_t nameOffset = boneNameOffsets[index];
            if (nameOffset >= names.size())
                throw std::runtime_error("Error reading mesh data: invalid bone name offset");
            const std::size_t terminator = names.find('\0', nameOffset);
            if (terminator == std::string_view::npos)
                throw std::runtime_error("Error reading mesh data: unterminated bone name");
            mesh.bones[index].name.assign(names.substr(nameOffset, terminator - nameOffset));
        }

        const std::uint32_t subsetCount = readU32(data, offset);
        checkCountFits(data, offset, subsetCount, 72, "subset");
        std::vector<RawSubset> subsets(subsetCount);
        for (RawSubset& subset : subsets)
        {
            subset.facesBegin = readU32(data, offset);
            subset.facesLength = readU32(data, offset);
            subset.vertsBegin = readU32(data, offset);
            subset.vertsLength = readU32(data, offset);
            subset.numBoneIndices = readU32(data, offset);
            readData(data, offset, subset.boneIndices.data(), 52);
            if (subset.numBoneIndices > subset.boneIndices.size() ||
				subset.facesBegin > mesh.faces.size() ||
				subset.facesLength > mesh.faces.size() - subset.facesBegin ||
				subset.vertsBegin > mesh.vnts.size() ||
				subset.vertsLength > mesh.vnts.size() - subset.vertsBegin)
                throw std::runtime_error("Error reading mesh data: invalid subset range");
			std::set<std::uint16_t> palette;
			for (std::size_t index = 0; index < subset.numBoneIndices; ++index)
				if (subset.boneIndices[index] >= boneCount ||
					!palette.insert(subset.boneIndices[index]).second)
					throw std::runtime_error("Error reading mesh data: invalid subset palette");
        }
        if (offset != end)
            throw std::runtime_error("Error reading mesh data: unexpected skinning data");

        mesh.skinning.resize(skinCount);
        mesh.subsets.reserve(subsets.size());
		std::vector<bool> coveredFaces(mesh.faces.size());
		std::vector<bool> coveredVertices(mesh.vnts.size());
        for (const RawSubset& subset : subsets)
        {
            FileMeshData::Subset decoded;
            decoded.facesBegin = subset.facesBegin;
            decoded.facesLength = subset.facesLength;
            decoded.vertsBegin = subset.vertsBegin;
            decoded.vertsLength = subset.vertsLength;
            decoded.boneIndices.assign(subset.boneIndices.begin(),
                subset.boneIndices.begin() + subset.numBoneIndices);
            for (std::size_t faceIndex = subset.facesBegin;
                faceIndex < subset.facesBegin + subset.facesLength; ++faceIndex)
            {
				if (coveredFaces[faceIndex])
					throw std::runtime_error("Error reading mesh data: overlapping subsets");
				coveredFaces[faceIndex] = true;
                const FileMeshFace& face = mesh.faces[faceIndex];
                const std::uint32_t vertexEnd = subset.vertsBegin + subset.vertsLength;
                if (face.a < subset.vertsBegin || face.a >= vertexEnd ||
                    face.b < subset.vertsBegin || face.b >= vertexEnd ||
                    face.c < subset.vertsBegin || face.c >= vertexEnd)
                    throw std::runtime_error(
                        "Error reading mesh data: subset face references an external vertex");
            }
            mesh.subsets.push_back(std::move(decoded));

            for (std::size_t vertexIndex = subset.vertsBegin;
                vertexIndex < subset.vertsBegin + subset.vertsLength; ++vertexIndex)
            {
				if (coveredVertices[vertexIndex])
					throw std::runtime_error("Error reading mesh data: overlapping subsets");
				coveredVertices[vertexIndex] = true;
                FileMeshData::Skinning& result = mesh.skinning[vertexIndex];
                result.boneWeights = rawSkinning[vertexIndex].weights;
				unsigned int totalWeight = 0;
                for (std::size_t influence = 0; influence < 4; ++influence)
                {
					totalWeight += result.boneWeights[influence];
                    if (result.boneWeights[influence] == 0)
                        continue;
                    const std::uint8_t subsetIndex =
                        rawSkinning[vertexIndex].subsetIndices[influence];
                    if (subsetIndex >= subset.numBoneIndices ||
                        subset.boneIndices[subsetIndex] >= mesh.bones.size())
                        throw std::runtime_error("Error reading mesh data: invalid skinning bone index");
                    result.boneIndices[influence] =
                        static_cast<std::uint8_t>(subset.boneIndices[subsetIndex]);
                }
				if (totalWeight != 255)
					throw std::runtime_error("Error reading mesh data: invalid skinning weights");
            }
        }
		if (std::find(coveredFaces.begin(), coveredFaces.end(), false) != coveredFaces.end() ||
			std::find(coveredVertices.begin(), coveredVertices.end(), false) != coveredVertices.end())
			throw std::runtime_error("Error reading mesh data: incomplete subset coverage");
    }

    static void readLegacySkinning(const std::string& data, std::size_t& offset,
        std::uint32_t vertexCount, std::uint16_t boneCount,
        std::uint32_t nameTableSize, std::uint16_t subsetCount,
        FileMeshData& mesh)
    {
        // v4/v5 store the same payload without the four leading counts used by
        // the chunked v6/v7 representation. Rebuild that small framing and use
        // the single strict decoder above.
        std::string framed;
        auto appendValue = [&framed](const auto& value) {
            framed.append(reinterpret_cast<const char*>(&value), sizeof(value));
        };
        appendValue(vertexCount);
        const std::size_t skinBytes = static_cast<std::size_t>(vertexCount) * 8;
        const std::size_t boneBytes = static_cast<std::size_t>(boneCount) * 60;
        const std::size_t subsetBytes = static_cast<std::size_t>(subsetCount) * 72;
        const std::size_t payloadBytes = skinBytes + boneBytes + nameTableSize + subsetBytes;
        if (offset + payloadBytes > data.size())
            throw std::runtime_error("Error reading mesh data: truncated legacy skinning");
        framed.append(data.data() + offset, skinBytes);
        appendValue(static_cast<std::uint32_t>(boneCount));
        framed.append(data.data() + offset + skinBytes, boneBytes);
        appendValue(nameTableSize);
        framed.append(data.data() + offset + skinBytes + boneBytes, nameTableSize);
        appendValue(static_cast<std::uint32_t>(subsetCount));
        framed.append(data.data() + offset + skinBytes + boneBytes + nameTableSize,
            subsetBytes);
        std::size_t framedOffset = 0;
        readSkinningPayload(framed, framedOffset, framed.size(), mesh);
        offset += payloadBytes;
    }

    static shared_ptr<FileMeshData> readMeshFromV4OrV5(
        const std::string& data, size_t offset, bool facs)
    {
        shared_ptr<FileMeshData> mesh(new FileMeshData());
        const std::uint16_t headerSize = readU16(data, offset);
        readU16(data, offset); // LOD generation type
        const std::uint32_t vertexCount = readU32(data, offset);
        const std::uint32_t faceCount = readU32(data, offset);
        const std::uint16_t lodCount = readU16(data, offset);
        const std::uint16_t boneCount = readU16(data, offset);
        const std::uint32_t nameTableSize = readU32(data, offset);
        const std::uint16_t subsetCount = readU16(data, offset);
        std::uint8_t highQualityLods = 0, padding = 0;
        readData(data, offset, &highQualityLods, 1);
        readData(data, offset, &padding, 1);
        std::uint32_t facsFormat = 0, facsSize = 0;
        const std::uint16_t requiredHeader = facs ? 32 : 24;
        if (facs)
        {
            facsFormat = readU32(data, offset);
            facsSize = readU32(data, offset);
        }
        if (headerSize < requiredHeader)
            throw std::runtime_error("Error reading mesh data: incompatible skinned mesh header");
        offset += headerSize - requiredHeader;

        readVertices(data, offset, vertexCount, kModernVertexSize, *mesh);
        if (boneCount)
        {
            // Save the interleaved legacy skin records until after faces/LODs.
            checkCountFits(data, offset, vertexCount, 8, "skinning");
            const std::string rawSkinning = data.substr(offset, vertexCount * 8);
            offset += vertexCount * 8;
            readFaces(data, offset, faceCount, sizeof(FileMeshFace), *mesh);
            checkCountFits(data, offset, lodCount, sizeof(std::uint32_t), "LOD");
            mesh->lodOffsets.resize(lodCount);
            for (std::uint32_t& lod : mesh->lodOffsets)
                lod = readU32(data, offset);

            // Reconstruct the legacy payload at its actual post-LOD position.
            const std::size_t bonesStart = offset;
            const std::size_t remaining = static_cast<std::size_t>(boneCount) * 60 +
                nameTableSize + static_cast<std::size_t>(subsetCount) * 72;
            if (bonesStart + remaining > data.size())
                throw std::runtime_error("Error reading mesh data: truncated legacy bones");
            std::string legacy = rawSkinning;
            legacy.append(data.data() + bonesStart, remaining);
            std::string framed;
            auto appendValue = [&framed](const auto& value) {
                framed.append(reinterpret_cast<const char*>(&value), sizeof(value));
            };
            appendValue(vertexCount);
            framed.append(rawSkinning);
            appendValue(static_cast<std::uint32_t>(boneCount));
            framed.append(data.data() + bonesStart, static_cast<std::size_t>(boneCount) * 60);
            appendValue(nameTableSize);
            framed.append(data.data() + bonesStart + static_cast<std::size_t>(boneCount) * 60,
                nameTableSize);
            appendValue(static_cast<std::uint32_t>(subsetCount));
            framed.append(data.data() + bonesStart + static_cast<std::size_t>(boneCount) * 60 +
                nameTableSize, static_cast<std::size_t>(subsetCount) * 72);
            std::size_t framedOffset = 0;
            readSkinningPayload(framed, framedOffset, framed.size(), *mesh);
            offset += remaining;
        }
        else
        {
            readFaces(data, offset, faceCount, sizeof(FileMeshFace), *mesh);
            mesh->lodOffsets.resize(lodCount);
            for (std::uint32_t& lod : mesh->lodOffsets)
                lod = readU32(data, offset);
        }
        if (facsFormat != 0 && facsSize != 0)
            offset += facsSize;
        if (offset != data.size())
            throw std::runtime_error("Error reading mesh data: unexpected data at end of file");
        validateFaces(*mesh);
        return mesh;
    }

    static const draco::PointAttribute* findAttribute(const draco::Mesh& source,
        draco::GeometryAttribute::Type type, std::uint8_t components,
        draco::DataType dataType = draco::DT_INVALID)
    {
        for (int index = 0; index < source.num_attributes(); ++index)
        {
            const draco::PointAttribute* attribute = source.attribute(index);
            if (attribute->attribute_type() == type &&
                attribute->num_components() == components &&
                (dataType == draco::DT_INVALID || attribute->data_type() == dataType))
                return attribute;
        }
        return nullptr;
    }

    template<typename T, int Components>
    static std::array<T, Components> readDracoAttribute(
        const draco::PointAttribute& attribute, draco::PointIndex point)
    {
        std::array<T, Components> result = {};
        if (!attribute.ConvertValue<T, Components>(
                attribute.mapped_index(point), result.data()))
            throw std::runtime_error("Error reading mesh data: Draco attribute conversion failed");
        return result;
    }

    static void readDracoCore(const char* bytes, std::size_t size, FileMeshData& mesh)
    {
        draco::DecoderBuffer buffer;
        buffer.Init(bytes, size);
        auto decoded = draco::Decoder().DecodeMeshFromBuffer(&buffer);
        if (!decoded.ok())
            throw std::runtime_error("Error reading mesh data: Draco decode failed: " +
                decoded.status().error_msg_string());
        const draco::Mesh& source = *decoded.value();
        const draco::PointAttribute* position =
            findAttribute(source, draco::GeometryAttribute::POSITION, 3);
        const draco::PointAttribute* normal =
            findAttribute(source, draco::GeometryAttribute::NORMAL, 3);
        if (!normal)
            normal = findAttribute(source, draco::GeometryAttribute::GENERIC, 3,
                draco::DT_FLOAT32);
        const draco::PointAttribute* uv =
            findAttribute(source, draco::GeometryAttribute::TEX_COORD, 2);
        const draco::PointAttribute* tangent =
            findAttribute(source, draco::GeometryAttribute::GENERIC, 4,
                draco::DT_INT8);
        const draco::PointAttribute* color =
            findAttribute(source, draco::GeometryAttribute::COLOR, 4);
        if (!position || !normal || !uv)
            throw std::runtime_error("Error reading mesh data: Draco core lacks required attributes");

        mesh.vnts.resize(source.num_points());
        mesh.vertexExtras.resize(source.num_points());
        for (draco::PointIndex point(0); point < source.num_points(); ++point)
        {
            const auto p = readDracoAttribute<float, 3>(*position, point);
            const auto n = readDracoAttribute<float, 3>(*normal, point);
            const auto t = readDracoAttribute<float, 2>(*uv, point);
            mesh.vnts[point.value()] = {p[0], p[1], p[2], n[0], n[1], n[2],
                t[0], t[1], 0.0f};
            if (tangent)
                mesh.vertexExtras[point.value()].tangent =
                    readDracoAttribute<std::int8_t, 4>(*tangent, point);
            if (color)
                mesh.vertexExtras[point.value()].color =
                    readDracoAttribute<std::uint8_t, 4>(*color, point);
        }
        mesh.faces.resize(source.num_faces());
        for (draco::FaceIndex index(0); index < source.num_faces(); ++index)
        {
            const draco::Mesh::Face& face = source.face(index);
            mesh.faces[index.value()] = {
                static_cast<std::uint32_t>(face[0].value()),
                static_cast<std::uint32_t>(face[1].value()),
                static_cast<std::uint32_t>(face[2].value())};
        }
    }

    static shared_ptr<FileMeshData> readMeshFromV6OrV7(
        const std::string& data, size_t offset, bool dracoCore)
    {
        shared_ptr<FileMeshData> mesh(new FileMeshData());
        while (offset < data.size())
        {
            if (data.size() - offset < 16)
                throw std::runtime_error("Error reading mesh data: truncated chunk header");
            char name[8];
            readData(data, offset, name, sizeof(name));
            const std::uint32_t version = readU32(data, offset);
            const std::uint32_t size = readU32(data, offset);
            const std::size_t end = offset + size;
            if (end < offset || end > data.size())
                throw std::runtime_error("Error reading mesh data: chunk is out of bounds");

            const std::string_view chunkName(name, sizeof(name));
            if (chunkName == std::string_view("COREMESH", 8))
            {
                if ((!dracoCore && version != 1) || (dracoCore && version != 2))
                    throw std::runtime_error("Error reading mesh data: unsupported COREMESH version");
                if (dracoCore)
                {
                    const std::uint32_t bitstreamSize = readU32(data, offset);
                    if (offset + bitstreamSize != end)
                        throw std::runtime_error("Error reading mesh data: invalid Draco payload size");
                    readDracoCore(data.data() + offset, bitstreamSize, *mesh);
                    offset += bitstreamSize;
                }
                else
                {
                    const std::uint32_t vertexCount = readU32(data, offset);
                    readVertices(data, offset, vertexCount, kModernVertexSize, *mesh);
                    const std::uint32_t faceCount = readU32(data, offset);
                    readFaces(data, offset, faceCount, sizeof(FileMeshFace), *mesh);
                }
            }
            else if (chunkName.substr(0, 4) == "LODS")
            {
                if (version != 1)
                    throw std::runtime_error("Error reading mesh data: unsupported LODS version");
                readU16(data, offset); // generation type
                std::uint8_t highQualityLods;
                readData(data, offset, &highQualityLods, 1);
                const std::uint32_t count = readU32(data, offset);
                mesh->lodOffsets.resize(count);
                for (std::uint32_t& lod : mesh->lodOffsets)
                    lod = readU32(data, offset);
            }
            else if (chunkName == std::string_view("SKINNING", 8))
            {
                if (version != 1 || mesh->vnts.empty())
                    throw std::runtime_error("Error reading mesh data: unsupported or misplaced SKINNING chunk");
                readSkinningPayload(data, offset, end, *mesh);
            }
            // FACS and HSRAVIS are retained by their owning avatar systems;
            // they do not alter renderable vertex/index data here.
            offset = end;
        }
        validateFaces(*mesh);
        if (!mesh->skinning.empty() && mesh->skinning.size() != mesh->vnts.size())
            throw std::runtime_error("Error reading mesh data: incomplete skinning data");
        return mesh;
    }

	FileMeshData* computeAABB(FileMeshData* mesh)
	{
		if (mesh->vnts.empty())
		{
			mesh->aabb = AABox(Vector3::zero());
		}
		else
		{
			AABox result = AABox(Vector3(mesh->vnts[0].vx, mesh->vnts[0].vy, mesh->vnts[0].vz));
			
			for (size_t i = 1; i < mesh->vnts.size(); ++i)
				result.merge(Vector3(mesh->vnts[i].vx, mesh->vnts[i].vy, mesh->vnts[i].vz));
				
			mesh->aabb = result;
		}
			
		return mesh;
	}
}

namespace RBX
{
    shared_ptr<FileMeshData> ReadFileMesh(const std::string& data)
    {
        std::string::size_type versionEnd = data.find('\n');
        if (versionEnd == std::string::npos)
            throw std::runtime_error("Error reading mesh data: unknown version");
  
        shared_ptr<FileMeshData> result;

        if (data.compare(0, 12, "version 1.00") == 0)
            result = readMeshFromV1(data, versionEnd + 1, 0.5f);
        else if (data.compare(0, 12, "version 1.01") == 0)
            result = readMeshFromV1(data, versionEnd + 1, 1.0f);
        else if (data.compare(0, 12, "version 2.00") == 0)
            result = readMeshFromV2(data, versionEnd + 1);
        else if (data.compare(0, 12, "version 3.00") == 0 ||
                data.compare(0, 12, "version 3.01") == 0)
            result = readMeshFromV3(data, versionEnd + 1);
        else if (data.compare(0, 12, "version 4.00") == 0 ||
                data.compare(0, 12, "version 4.01") == 0)
            result = readMeshFromV4OrV5(data, versionEnd + 1, false);
        else if (data.compare(0, 12, "version 5.00") == 0)
            result = readMeshFromV4OrV5(data, versionEnd + 1, true);
        else if (data.compare(0, 12, "version 6.00") == 0)
            result = readMeshFromV6OrV7(data, versionEnd + 1, false);
        else if (data.compare(0, 12, "version 7.00") == 0)
            result = readMeshFromV6OrV7(data, versionEnd + 1, true);
        else
            throw std::runtime_error("Error reading mesh data: unknown version");

        computeAABB(result.get());
        
        return result;
    }

	void WriteFileMesh(std::ostream& f, const FileMeshData& data)
	{
		f << "version 2.00" << std::endl;
		
		FileMeshHeader header;
		header.num_faces = data.faces.size();
		header.num_vertices = data.vnts.size();
		header.cbFaceStride = (unsigned char)sizeof(data.faces[0]);
		header.cbVerticesStride = (unsigned char)sizeof(data.vnts[0]);
		header.cbSize = sizeof(header);
		f.write(reinterpret_cast<char*>(&header), sizeof(header));

		f.write(reinterpret_cast<const char*>(&data.vnts[0]), sizeof(data.vnts[0]) * data.vnts.size());
		f.write(reinterpret_cast<const char*>(&data.faces[0]), sizeof(data.faces[0]) * data.faces.size());
	}
}
