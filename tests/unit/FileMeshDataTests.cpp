#include "GfxBase/FileMeshData.h"

#include <cstring>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

namespace
{

template<typename T>
void append(std::string& destination, const T& value)
{
    destination.append(reinterpret_cast<const char*>(&value), sizeof(value));
}

std::string makeExtendedMesh()
{
    std::string data = "version 2.00\n";

    RBX::FileMeshHeader header = {};
    header.cbSize = sizeof(header) + 2;
    header.cbVerticesStride = sizeof(RBX::FileMeshVertexNormalTexture3d) + 4;
    header.cbFaceStride = sizeof(RBX::FileMeshFace) + 4;
    header.num_vertices = 3;
    header.num_faces = 1;
    append(data, header);
    data.append(2, '\0');

    for (unsigned int index = 0; index < 3; ++index)
    {
        RBX::FileMeshVertexNormalTexture3d vertex = {};
        vertex.vx = static_cast<float>(index + 1);
        vertex.ny = 1.0f;
        append(data, vertex);
        const unsigned int appendedVertexField = 0x12345678U + index;
        append(data, appendedVertexField);
    }

    const RBX::FileMeshFace face = {0, 1, 2};
    append(data, face);
    const unsigned int appendedFaceField = 0x87654321U;
    append(data, appendedFaceField);
    return data;
}

} // namespace

int main(int argc, char** argv)
{
    const std::string data = makeExtendedMesh();
    const boost::shared_ptr<RBX::FileMeshData> mesh = RBX::ReadFileMesh(data);
    if (!mesh || mesh->vnts.size() != 3 || mesh->faces.size() != 1 ||
        mesh->vnts[0].vx != 1.0f || mesh->vnts[2].vx != 3.0f ||
        mesh->faces[0].a != 0 || mesh->faces[0].b != 1 ||
        mesh->faces[0].c != 2)
        throw std::runtime_error("extended v2 mesh records were not read correctly");

    bool rejectedTruncatedMesh = false;
    try
    {
        RBX::ReadFileMesh(data.substr(0, data.size() - 1));
    }
    catch (const std::runtime_error&)
    {
        rejectedTruncatedMesh = true;
    }
    if (!rejectedTruncatedMesh)
        throw std::runtime_error("truncated extended mesh record was accepted");

    if (argc == 2)
    {
        std::ifstream input(argv[1], std::ios::binary);
        const std::string modernData((std::istreambuf_iterator<char>(input)), {});
        const boost::shared_ptr<RBX::FileMeshData> modern =
            RBX::ReadFileMesh(modernData);
        bool hasHeadBase = false;
        for (const RBX::FileMeshData::Bone& bone : modern->bones)
            hasHeadBase = hasHeadBase || bone.name == "HeadBase";
        if (!modern || modern->vnts.size() != 2369 ||
            modern->faces.size() != 3991 ||
            modern->vertexExtras.size() != modern->vnts.size() ||
            modern->skinning.size() != modern->vnts.size() ||
            modern->bones.size() != 45 || modern->bones.front().name != "Root" ||
			modern->subsets.size() != 3 ||
			modern->subsets[0].boneIndices.size() != 26 ||
			modern->subsets[1].boneIndices.size() != 26 ||
			modern->subsets[2].boneIndices.size() != 15 ||
            !hasHeadBase)
            throw std::runtime_error("current skinned FileMesh v7 contract changed");
    }

    return 0;
}
