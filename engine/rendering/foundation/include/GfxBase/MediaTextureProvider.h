#pragma once

#include "GfxBase/TextureProxyBase.h"

#include <cstddef>
#include <cstdint>

namespace RBX {

class MediaTextureProvider
{
public:
    virtual ~MediaTextureProvider() {}
    virtual TextureProxyBaseRef requestMediaTexture(const void* owner,
        std::uint64_t generation, unsigned int width, unsigned int height,
        const std::uint8_t* rgba, std::size_t byteCount) = 0;
};

} // namespace RBX
