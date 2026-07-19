#include "GfxCore/States.h"
#include "GfxCore/Texture.h"

#include <cassert>

int main() {
    using namespace RBX::Graphics;

    static_assert(Texture::Format_Count == 16);
    assert(Texture::getImageSize(Texture::Format_RGBA8, 4, 8) == 128);
    assert(Texture::getImageSize(Texture::Format_BC1, 7, 5) == 32);
    assert(Texture::getImageSize(Texture::Format_PVRTC_RGBA2, 4, 4) == 32);
    assert(Texture::getMipSide(17, 2) == 4);
    assert(Texture::getMaxMipCount(8, 4, 1) == 4);
    assert(Texture::isFormatCompressed(Texture::Format_BC3));
    assert(Texture::isFormatDepth(Texture::Format_D24S8));

    const RasterizerState back(RasterizerState::Cull_Back);
    const RasterizerState front(RasterizerState::Cull_Front);
    assert(back != front);
    assert(back.getHashId() != front.getHashId());

    const BlendState alpha(BlendState::Mode_AlphaBlend);
    assert(alpha.blendingNeeded());
    assert(alpha.getColorSrc() == BlendState::Factor_SrcAlpha);
    assert(alpha.getColorDst() == BlendState::Factor_InvSrcAlpha);

    const DepthState depth(DepthState::Function_LessEqual, true);
    assert(depth.getFunction() == DepthState::Function_LessEqual);
    assert(depth.getWrite());

    const SamplerState sampler(SamplerState::Filter_Anisotropic,
                               SamplerState::Address_Clamp, 8);
    assert(sampler.getFilter() == SamplerState::Filter_Anisotropic);
    assert(sampler.getAddress() == SamplerState::Address_Clamp);
    assert(sampler.getAnisotropy() == 8);
    return 0;
}
