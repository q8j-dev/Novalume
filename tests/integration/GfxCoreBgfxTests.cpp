#include "GfxCore/Device.h"
#include "GfxCore/Framebuffer.h"
#include "GfxCore/States.h"
#include "rbx/platform/MacHost.h"

#include <filesystem>
#include <memory>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

static std::vector<char> readBinary(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream) throw std::runtime_error("unable to open test shader: " + path.string());
    const auto size = stream.tellg();
    std::vector<char> result(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(result.data(), size);
    return result;
}

int main() {
    auto host = rbx::platform::createMacHost(320, 180, false);
    const auto surface = host->nativeSurface();
    const RBX::Graphics::DeviceWindow deviceWindow{
        .windowHandle = reinterpret_cast<void*>(surface.window),
        .displayHandle = reinterpret_cast<void*>(surface.display),
        .graphicsContext = reinterpret_cast<void*>(surface.graphicsContext),
        .width = surface.width,
        .height = surface.height,
        .pixelDensity = surface.pixelDensity};
    std::unique_ptr<RBX::Graphics::Device> device(
        RBX::Graphics::Device::create(RBX::Graphics::Device::API_Bgfx, deviceWindow));
    if (!device->validate() || device->getAPIName() != "bgfx" ||
        device->getFeatureLevel() != "Metal") {
        throw std::runtime_error("historical GfxCore did not select bgfx Metal");
    }
    if (!device->getMainFramebuffer() ||
        device->getMainFramebuffer()->getWidth() != surface.width ||
        device->getMainFramebuffer()->getHeight() != surface.height) {
        throw std::runtime_error("bgfx GfxCore main framebuffer dimensions are invalid");
    }
    auto* context = device->beginFrame();
    {
        using namespace RBX::Graphics;
        std::vector<VertexLayout::Element> elements;
        elements.emplace_back(0, 0, VertexLayout::Format_Float3,
                              VertexLayout::Semantic_Position);
        elements.emplace_back(0, 12, VertexLayout::Format_Color,
                              VertexLayout::Semantic_Color);
        auto layout = device->createVertexLayout(elements);
        auto vertices = device->createVertexBuffer(16, 3, GeometryBuffer::Usage_Dynamic);
        const float vertexData[12] = {
            0.0F, 0.5F, 0.0F, 1.0F,
            -0.5F, -0.5F, 0.0F, 1.0F,
            0.5F, -0.5F, 0.0F, 1.0F};
        vertices->upload(0, vertexData, sizeof(vertexData));
        auto indices = device->createIndexBuffer(2, 3, GeometryBuffer::Usage_Dynamic);
        const std::uint16_t indexData[] = {0, 1, 2};
        indices->upload(0, indexData, sizeof(indexData));
        auto geometry = device->createGeometry(layout, vertices, indices);
        if (!geometry) throw std::runtime_error("bgfx GfxCore geometry creation failed");

        auto vertexShader = device->createVertexShader(
            readBinary(std::filesystem::path(RBX_TEST_SHADER_DIR) / "vs_player_ui.sc.bin"));
        auto fragmentShader = device->createFragmentShader(
            readBinary(std::filesystem::path(RBX_TEST_SHADER_DIR) / "fs_player_ui.sc.bin"));
        auto program = device->createShaderProgram(vertexShader, fragmentShader);
        if (!program || program->getSamplerMask() != 1U)
            throw std::runtime_error("bgfx GfxCore shader reflection failed");
        context->bindProgram(program.get());
        context->setScissor(0, 0, 160, 90);

        auto texture = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8,
            4, 4, 1, 1, Texture::Usage_Dynamic);
        std::vector<std::uint8_t> pixels(4 * 4 * 4, 0x7f);
        texture->upload(0, 0, TextureRegion(0, 0, 4, 4), pixels.data(),
                        static_cast<unsigned int>(pixels.size()));
        const TextureRegion lockRegion(1, 1, 2, 2);
        Texture::LockResult locked = texture->lock(0, 0, lockRegion);
        if (!locked.data || locked.rowPitch != 8 || locked.slicePitch != 16)
            throw std::runtime_error("bgfx GfxCore texture locking failed");
        std::fill_n(static_cast<std::uint8_t*>(locked.data), 16, 0xff);
        texture->unlock(0, 0);
        std::vector<std::uint8_t> downloaded(pixels.size());
        if (!texture->download(0, 0, downloaded.data(),
                               static_cast<unsigned int>(downloaded.size())) ||
            downloaded[0] != 0x7f || downloaded[(1 * 4 + 1) * 4] != 0xff)
            throw std::runtime_error("bgfx GfxCore texture readback failed");
        auto mipTexture = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8,
            4, 4, 1, 3, Texture::Usage_Static);
        bool rejectedRuntimeMipGeneration = false;
        try {
            mipTexture->generateMipmaps();
        } catch (const std::runtime_error&) {
            rejectedRuntimeMipGeneration = true;
        }
        if (!rejectedRuntimeMipGeneration)
            throw std::runtime_error("bgfx GfxCore mip generation policy was not enforced");
        context->bindTexture(0, texture.get(),
                             SamplerState(SamplerState::Filter_Linear,
                                          SamplerState::Address_Clamp));

        if (!context->drawTransient(layout.get(), Geometry::Primitive_Triangles,
                                    vertexData, 16, 3, indexData, 2, 3))
            throw std::runtime_error("bgfx transient geometry allocation failed");
        context->clearScissor();

        auto targetTexture = device->createTexture(Texture::Type_2D,
            Texture::Format_RGBA8, 16, 16, 1, 1, Texture::Usage_Renderbuffer);
        auto target = device->createFramebuffer(targetTexture->getRenderbuffer(0, 0));
        if (!target || target->getWidth() != 16 || target->getHeight() != 16)
            throw std::runtime_error("bgfx GfxCore framebuffer creation failed");
        context->bindFramebuffer(target.get());
        const float targetColor[] = {0.25F, 0.5F, 0.75F, 1.0F};
        context->clearFramebuffer(DeviceContext::Buffer_Color, targetColor, 1.0F, 0);
        context->bindFramebuffer(nullptr);
        context->bindProgram(nullptr);
    }
    const float color[] = {0.1F, 0.2F, 0.3F, 1.0F};
    context->clearFramebuffer(RBX::Graphics::DeviceContext::Buffer_Color |
                                  RBX::Graphics::DeviceContext::Buffer_Depth,
                              color, 1.0F, 0);
    device->endFrame();
    device->beginFrame();
    device->endFrame();

    device->resize(256, 144, 1.0F);
    if (device->getMainFramebuffer()->getWidth() != 256 ||
        device->getMainFramebuffer()->getHeight() != 144)
        throw std::runtime_error("bgfx GfxCore resize did not update the main framebuffer");

    std::vector<std::uint8_t> screenshot(256U * 144U * 4U);
    device->getMainFramebuffer()->download(screenshot.data(),
        static_cast<unsigned int>(screenshot.size()));

    device->suspend();
    if (device->validate())
        throw std::runtime_error("bgfx GfxCore remained valid while suspended");
    bool rejectedSuspendedFrame = false;
    try {
        device->beginFrame();
    } catch (const std::logic_error&) {
        rejectedSuspendedFrame = true;
    }
    if (!rejectedSuspendedFrame)
        throw std::runtime_error("bgfx GfxCore accepted a frame while suspended");
    device->resume();
    if (!device->validate())
        throw std::runtime_error("bgfx GfxCore did not restore after resume");
    return 0;
}
