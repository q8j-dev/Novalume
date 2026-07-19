#pragma once

#include "GfxCore/Device.h"

#include <memory>

namespace RBX::Graphics {

// Direct implementation of the historical GfxCore seam. Resource creation is
// being migrated incrementally; the class is deliberately in GfxCore so
// VisualEngine uses bgfx without a parallel scene renderer.
class DeviceBgfx final : public Device {
public:
    explicit DeviceBgfx(const DeviceWindow& window);
    ~DeviceBgfx() override;

    bool validate() override;
    DeviceContext* beginFrame() override;
    void endFrame() override;
    void resize(unsigned int width, unsigned int height, float pixelDensity) override;
    Framebuffer* getMainFramebuffer() override;
    DeviceVR* getVR() override;
    void setVR(bool enabled) override;
    void defineGlobalConstants(size_t dataSize,
                               const std::vector<ShaderGlobalConstant>& constants) override;
    std::string getAPIName() override;
    std::string getFeatureLevel() override;
    std::string getShadingLanguage() override;
    std::string createShaderSource(const std::string& path,
        const std::string& defines,
        std::function<std::string(const std::string&)> fileCallback) override;
    std::vector<char> createShaderBytecode(const std::string& source,
        const std::string& target, const std::string& entrypoint) override;
    shared_ptr<VertexShader> createVertexShader(const std::vector<char>& bytecode) override;
    shared_ptr<FragmentShader> createFragmentShader(const std::vector<char>& bytecode) override;
    shared_ptr<ShaderProgram> createShaderProgram(
        const shared_ptr<VertexShader>& vertexShader,
        const shared_ptr<FragmentShader>& fragmentShader) override;
    shared_ptr<ShaderProgram> createShaderProgramFFP() override;
    shared_ptr<VertexBuffer> createVertexBuffer(size_t elementSize,
        size_t elementCount, GeometryBuffer::Usage usage) override;
    shared_ptr<IndexBuffer> createIndexBuffer(size_t elementSize,
        size_t elementCount, GeometryBuffer::Usage usage) override;
    shared_ptr<VertexLayout> createVertexLayout(
        const std::vector<VertexLayout::Element>& elements) override;
    shared_ptr<Texture> createTexture(Texture::Type type, Texture::Format format,
        unsigned int width, unsigned int height, unsigned int depth,
        unsigned int mipLevels, Texture::Usage usage) override;
    shared_ptr<Renderbuffer> createRenderbuffer(Texture::Format format,
        unsigned int width, unsigned int height, unsigned int samples) override;
    const DeviceCaps& getCaps() const override;
    DeviceStats getStatistics() const override;
    void suspend() override;
    void resume() override;

protected:
    shared_ptr<Geometry> createGeometryImpl(const shared_ptr<VertexLayout>& layout,
        const std::vector<shared_ptr<VertexBuffer>>& vertexBuffers,
        const shared_ptr<IndexBuffer>& indexBuffer,
        unsigned int baseVertexIndex) override;
    shared_ptr<Framebuffer> createFramebufferImpl(
        const std::vector<shared_ptr<Renderbuffer>>& color,
        const shared_ptr<Renderbuffer>& depth) override;

private:
    class Context;
    class Callback;
    std::unique_ptr<Context> context;
    std::unique_ptr<Callback> callback;
    shared_ptr<Framebuffer> mainFramebuffer;
    DeviceCaps caps{};
    unsigned int width = 0;
    unsigned int height = 0;
    std::uint32_t resetFlags = 0;
    bool initialized = false;
    bool suspended = false;
    bool frameActive = false;
};

} // namespace RBX::Graphics
