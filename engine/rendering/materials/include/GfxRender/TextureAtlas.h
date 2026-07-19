#pragma once
#include "TextureRef.h"
#include "GfxCore/Texture.h"
#include "g3d/Vector2int16.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace RBX
{
namespace Graphics
{

class VisualEngine;

class TextureAtlas
{
public: 
    TextureAtlas(VisualEngine* visualEngine, unsigned width, unsigned height);
    ~TextureAtlas();

    bool getRegion(const std::uint64_t& id, const TextureRegion*& region);
    bool insert(const std::uint64_t& id, const G3D::Vector2int16& size, unsigned dataPitch, unsigned char* data, const TextureRegion*& outRegion);

    const G3D::Vector2int16& getLastFailedSize() { return lastFailedSize; }

    const shared_ptr<Texture>& getTexture() const { return texture.getTexture(); }

    void upload();

private:
    void reset();

    struct Vector2uint32
    {
        Vector2uint32() : x(0), y(0) {}
        Vector2uint32(unsigned xi, unsigned yi)
            : x(xi)
            , y(yi){}
        unsigned x;
        unsigned y;
    };

    unsigned char* textureMirror;
    unsigned updateWindowPosition;
    unsigned char* updateWindow;
    
    TextureRef texture;
    Vector2uint32 textureSize;
    using AtlasEntries = std::unordered_map<std::uint64_t, TextureRegion>;
    AtlasEntries atlasEntries;

    G3D::Vector2int16 lastFailedSize;

    typedef std::vector<Vector2uint32> SkylineArray;
    SkylineArray skyline;
    
    unsigned moduloParam; // fast modulo by power of two numbers (number & (potNr - 1)) = number % potNr

    unsigned getUpdateWindowIdx(int x, int y);

    struct Interval1D
    {
        Interval1D(unsigned min, unsigned max): min(min), max(max) {}
        unsigned min, max;
    };
    Interval1D uploadRect;
    
    unsigned updateLineY;
};

}
}
