#include "bgfx/DeviceBgfx.h"
#include "GfxCore/Framebuffer.h"
#include "GfxCore/States.h"

#include <bgfx/bgfx.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdarg>
#include <cstring>
#include <cstdio>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace RBX::Graphics {
namespace {

struct GlobalConstantBinding final {
    std::string name;
    std::size_t offset = 0;
    std::size_t size = 0;
};

[[noreturn]] void notMigrated(const char* resource) {
    throw std::runtime_error(std::string("bgfx GfxCore migration incomplete: ") + resource);
}

bgfx::Attrib::Enum toAttrib(VertexLayout::Semantic semantic, unsigned int index) {
    switch (semantic) {
    case VertexLayout::Semantic_Position:
        if (index == 0) return bgfx::Attrib::Position;
        break;
    case VertexLayout::Semantic_Normal:
        if (index == 0) return bgfx::Attrib::Normal;
        break;
    case VertexLayout::Semantic_Color:
        if (index == 0) return bgfx::Attrib::Color0;
        if (index == 1) return bgfx::Attrib::Color1;
        break;
    case VertexLayout::Semantic_Texture:
        if (index < 8) return static_cast<bgfx::Attrib::Enum>(bgfx::Attrib::TexCoord0 + index);
        break;
    default:
        break;
    }
    throw std::invalid_argument("unsupported GfxCore vertex semantic");
}

struct FormatInfo final {
    std::uint8_t components;
    bgfx::AttribType::Enum type;
    bool normalized;
    std::uint16_t bytes;
};

FormatInfo formatInfo(VertexLayout::Format format) {
    switch (format) {
    case VertexLayout::Format_Float1: return {1, bgfx::AttribType::Float, false, 4};
    case VertexLayout::Format_Float2: return {2, bgfx::AttribType::Float, false, 8};
    case VertexLayout::Format_Float3: return {3, bgfx::AttribType::Float, false, 12};
    case VertexLayout::Format_Float4: return {4, bgfx::AttribType::Float, false, 16};
    case VertexLayout::Format_Short2: return {2, bgfx::AttribType::Int16, false, 4};
    case VertexLayout::Format_Short4: return {4, bgfx::AttribType::Int16, false, 8};
    case VertexLayout::Format_UByte4: return {4, bgfx::AttribType::Uint8, false, 4};
    case VertexLayout::Format_Color: return {4, bgfx::AttribType::Uint8, true, 4};
    default: throw std::invalid_argument("unsupported GfxCore vertex format");
    }
}

bgfx::TextureFormat::Enum textureFormat(Texture::Format format) {
    switch (format) {
    case Texture::Format_L8: return bgfx::TextureFormat::R8;
    case Texture::Format_LA8: return bgfx::TextureFormat::RG8;
    case Texture::Format_RGB5A1: return bgfx::TextureFormat::RGB5A1;
    case Texture::Format_RGBA8: return bgfx::TextureFormat::RGBA8;
    case Texture::Format_RG16: return bgfx::TextureFormat::RG16;
    case Texture::Format_RGBA16F: return bgfx::TextureFormat::RGBA16F;
    case Texture::Format_BC1: return bgfx::TextureFormat::BC1;
    case Texture::Format_BC2: return bgfx::TextureFormat::BC2;
    case Texture::Format_BC3: return bgfx::TextureFormat::BC3;
    case Texture::Format_PVRTC_RGB2: return bgfx::TextureFormat::PTC12;
    case Texture::Format_PVRTC_RGBA2: return bgfx::TextureFormat::PTC12A;
    case Texture::Format_PVRTC_RGB4: return bgfx::TextureFormat::PTC14;
    case Texture::Format_PVRTC_RGBA4: return bgfx::TextureFormat::PTC14A;
    case Texture::Format_ETC1: return bgfx::TextureFormat::ETC1;
    case Texture::Format_D16: return bgfx::TextureFormat::D16;
    case Texture::Format_D24S8: return bgfx::TextureFormat::D24S8;
    default: throw std::invalid_argument("unsupported GfxCore texture format");
    }
}

std::uint64_t multisampleFlags(unsigned int samples) {
    switch (samples) {
    case 1: return BGFX_TEXTURE_RT;
    case 2: return BGFX_TEXTURE_RT_MSAA_X2;
    case 4: return BGFX_TEXTURE_RT_MSAA_X4;
    case 8: return BGFX_TEXTURE_RT_MSAA_X8;
    case 16: return BGFX_TEXTURE_RT_MSAA_X16;
    default: throw std::invalid_argument("renderbuffer sample count must be 1, 2, 4, 8, or 16");
    }
}

void checkTextureExtent(unsigned int value, const char* label) {
    if (value == 0 || value > std::numeric_limits<std::uint16_t>::max())
        throw std::invalid_argument(std::string(label) + " exceeds the bgfx texture limit");
}

bgfx::RendererType::Enum rendererType(DeviceWindow::Renderer renderer) {
    switch (renderer) {
    case DeviceWindow::Renderer::Default: return bgfx::RendererType::Count;
    case DeviceWindow::Renderer::Direct3D11: return bgfx::RendererType::Direct3D11;
    case DeviceWindow::Renderer::Metal: return bgfx::RendererType::Metal;
    case DeviceWindow::Renderer::OpenGL: return bgfx::RendererType::OpenGL;
    case DeviceWindow::Renderer::Vulkan: return bgfx::RendererType::Vulkan;
    }
    return bgfx::RendererType::Count;
}

bool readTexture2D(bgfx::TextureHandle source, std::uint8_t sourceMip,
                   std::uint16_t sourceLayer, bgfx::TextureFormat::Enum format,
                   std::uint16_t width, std::uint16_t height, void* data,
                   std::size_t size) {
    const bgfx::Caps* caps = bgfx::getCaps();
    if ((caps->supported & BGFX_CAPS_TEXTURE_BLIT) == 0U ||
        (caps->supported & BGFX_CAPS_TEXTURE_READ_BACK) == 0U)
        return false;
    bgfx::TextureHandle staging = bgfx::createTexture2D(
        width, height, false, 1, format,
        BGFX_TEXTURE_BLIT_DST | BGFX_TEXTURE_READ_BACK);
    if (!bgfx::isValid(staging)) return false;
    bgfx::blit(255, staging, 0, 0, 0, 0, source, sourceMip, 0, 0,
               sourceLayer, width, height, 1);
    const std::uint32_t ready = bgfx::readTexture(staging, data);
    std::uint32_t frame = bgfx::frame();
    while (frame < ready) frame = bgfx::frame();
    bgfx::destroy(staging);
    (void)size;
    return true;
}

} // namespace

class DeviceBgfx::Callback final : public bgfx::CallbackI {
public:
    void fatal(const char* filePath, std::uint16_t line, bgfx::Fatal::Enum code,
               const char* message) override {
        std::fprintf(stderr, "bgfx fatal %s:%u: %s\n", filePath ? filePath : "?",
                     static_cast<unsigned int>(line), message ? message : "unknown error");
        if (code != bgfx::Fatal::DebugCheck)
            std::abort();
    }

    void traceVargs(const char*, std::uint16_t, const char* format,
                    va_list arguments) override {
        if (format)
            std::vfprintf(stderr, format, arguments);
    }

    void profilerBegin(const char*, std::uint32_t, const char*, std::uint16_t) override {}
    void profilerBeginLiteral(const char*, std::uint32_t, const char*, std::uint16_t) override {}
    void profilerEnd() override {}
    std::uint32_t cacheReadSize(std::uint64_t) override { return 0; }
    bool cacheRead(std::uint64_t, void*, std::uint32_t) override { return false; }
    void cacheWrite(std::uint64_t, const void*, std::uint32_t) override {}

    void screenShot(const char*, std::uint32_t screenshotWidth,
                    std::uint32_t screenshotHeight, std::uint32_t pitch,
                    bgfx::TextureFormat::Enum, const void* data, std::uint32_t,
                    bool yflip) override {
        std::lock_guard<std::mutex> lock(mutex);
        if (!capturePending || screenshotWidth != width || screenshotHeight != height || !data) {
            captureFailed = true;
            captureComplete = true;
            condition.notify_all();
            return;
        }

        const auto* source = static_cast<const std::uint8_t*>(data);
        for (std::uint32_t y = 0; y < height; ++y) {
            const std::uint32_t sourceY = yflip ? height - 1U - y : y;
            const std::uint8_t* sourceRow = source + sourceY * pitch;
            std::uint8_t* destinationRow = destination + y * width * 4U;
            for (std::uint32_t x = 0; x < width; ++x) {
                destinationRow[x * 4U + 0U] = sourceRow[x * 4U + 2U];
                destinationRow[x * 4U + 1U] = sourceRow[x * 4U + 1U];
                destinationRow[x * 4U + 2U] = sourceRow[x * 4U + 0U];
                destinationRow[x * 4U + 3U] = sourceRow[x * 4U + 3U];
            }
        }
        captureComplete = true;
        condition.notify_all();
    }

    void captureBegin(std::uint32_t, std::uint32_t, std::uint32_t,
                      bgfx::TextureFormat::Enum, bool) override {}
    void captureEnd() override {}
    void captureFrame(const void*, std::uint32_t) override {}

    void captureMainFramebuffer(std::uint32_t captureWidth, std::uint32_t captureHeight,
                                void* output, std::uint32_t size) {
        if (!output || size != captureWidth * captureHeight * 4U)
            throw std::invalid_argument("main framebuffer download requires RGBA8-sized output");
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (capturePending)
                throw std::logic_error("a main framebuffer capture is already pending");
            destination = static_cast<std::uint8_t*>(output);
            width = captureWidth;
            height = captureHeight;
            capturePending = true;
            captureComplete = false;
            captureFailed = false;
        }

        bgfx::requestScreenShot(BGFX_INVALID_HANDLE, "GfxCoreMainFramebuffer");
        for (unsigned int attempt = 0; attempt < 8; ++attempt) {
            bgfx::frame();
            std::unique_lock<std::mutex> lock(mutex);
            if (condition.wait_for(lock, std::chrono::milliseconds(250),
                                   [this] { return captureComplete; }))
                break;
        }

        std::lock_guard<std::mutex> lock(mutex);
        capturePending = false;
        destination = nullptr;
        if (!captureComplete || captureFailed)
            throw std::runtime_error("bgfx main framebuffer capture did not complete");
    }

private:
    std::mutex mutex;
    std::condition_variable condition;
    std::uint8_t* destination = nullptr;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    bool capturePending = false;
    bool captureComplete = false;
    bool captureFailed = false;
};

class VertexLayoutBgfx final : public VertexLayout {
public:
    VertexLayoutBgfx(Device* device, const std::vector<Element>& elements)
        : VertexLayout(device, elements) {}

    bgfx::VertexLayout build(unsigned int stream, std::size_t stride) const {
        if (stride > std::numeric_limits<std::uint16_t>::max())
            throw std::invalid_argument("vertex stride exceeds bgfx limit");
        std::vector<Element> selected;
        for (const Element& element : getElements())
            if (element.stream == stream) selected.push_back(element);
        std::sort(selected.begin(), selected.end(), [](const Element& left, const Element& right) {
            return left.offset < right.offset;
        });
        bgfx::VertexLayout result;
        result.begin();
        std::uint16_t cursor = 0;
        for (const Element& element : selected) {
            const FormatInfo info = formatInfo(element.format);
            if (element.offset < cursor || element.offset + info.bytes > stride)
                throw std::invalid_argument("overlapping or out-of-range vertex element");
            if (element.offset > cursor)
                result.skip(static_cast<std::uint8_t>(element.offset - cursor));
            result.add(toAttrib(element.semantic, element.semanticIndex), info.components,
                       info.type, info.normalized);
            cursor = static_cast<std::uint16_t>(element.offset + info.bytes);
        }
        if (stride > cursor) {
            const std::size_t padding = stride - cursor;
            if (padding > std::numeric_limits<std::uint8_t>::max())
                throw std::invalid_argument("vertex padding exceeds bgfx limit");
            result.skip(static_cast<std::uint8_t>(padding));
        }
        result.end();
        return result;
    }
};

class VertexBufferBgfx final : public VertexBuffer {
public:
    VertexBufferBgfx(Device* device, std::size_t elementSize, std::size_t elementCount,
                     Usage usage)
        : VertexBuffer(device, elementSize, elementCount, usage),
          bytes(elementSize * elementCount) {}
    ~VertexBufferBgfx() override {
        if (bgfx::isValid(handle)) bgfx::destroy(handle);
    }
    void* lock(LockMode mode) override {
        if (locked) throw std::logic_error("vertex buffer is already locked");
        if (mode == Lock_Discard) std::fill(bytes.begin(), bytes.end(), std::byte{});
        locked = true;
        return bytes.data();
    }
    void unlock() override {
        if (!locked) throw std::logic_error("vertex buffer is not locked");
        locked = false;
        updateGpu(0, bytes.data(), bytes.size());
    }
    void upload(unsigned int offset, const void* data, unsigned int size) override {
        if (locked || static_cast<std::size_t>(offset) + size > bytes.size())
            throw std::out_of_range("vertex buffer upload is out of range");
        std::memcpy(bytes.data() + offset, data, size);
        updateGpu(offset, data, size);
    }
    void ensure(const bgfx::VertexLayout& layout) {
        if (bgfx::isValid(handle)) return;
        handle = bgfx::createDynamicVertexBuffer(static_cast<std::uint32_t>(getElementCount()),
                                                  layout, BGFX_BUFFER_NONE);
        if (!bgfx::isValid(handle)) throw std::runtime_error("bgfx vertex buffer creation failed");
        updateGpu(0, bytes.data(), bytes.size());
    }
    bgfx::DynamicVertexBufferHandle getHandle() const { return handle; }

private:
    void updateGpu(std::size_t byteOffset, const void* data, std::size_t size) {
        if (!bgfx::isValid(handle) || size == 0) return;
        if (byteOffset % getElementSize() != 0 || size % getElementSize() != 0)
            throw std::invalid_argument("bgfx vertex updates must be vertex aligned");
        bgfx::update(handle, static_cast<std::uint32_t>(byteOffset / getElementSize()),
                     bgfx::copy(data, static_cast<std::uint32_t>(size)));
    }
    std::vector<std::byte> bytes;
    bgfx::DynamicVertexBufferHandle handle = BGFX_INVALID_HANDLE;
    bool locked = false;
};

class IndexBufferBgfx final : public IndexBuffer {
public:
    IndexBufferBgfx(Device* device, std::size_t elementSize, std::size_t elementCount,
                    Usage usage)
        : IndexBuffer(device, elementSize, elementCount, usage),
          bytes(elementSize * elementCount) {
        if (elementSize != 2 && elementSize != 4)
            throw std::invalid_argument("index elements must be 16 or 32 bit");
        handle = bgfx::createDynamicIndexBuffer(static_cast<std::uint32_t>(elementCount),
            elementSize == 4 ? BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE);
        if (!bgfx::isValid(handle)) throw std::runtime_error("bgfx index buffer creation failed");
    }
    ~IndexBufferBgfx() override { if (bgfx::isValid(handle)) bgfx::destroy(handle); }
    void* lock(LockMode mode) override {
        if (locked) throw std::logic_error("index buffer is already locked");
        if (mode == Lock_Discard) std::fill(bytes.begin(), bytes.end(), std::byte{});
        locked = true;
        return bytes.data();
    }
    void unlock() override {
        if (!locked) throw std::logic_error("index buffer is not locked");
        locked = false;
        uploadGpu(0, bytes.data(), bytes.size());
    }
    void upload(unsigned int offset, const void* data, unsigned int size) override {
        if (locked || static_cast<std::size_t>(offset) + size > bytes.size())
            throw std::out_of_range("index buffer upload is out of range");
        std::memcpy(bytes.data() + offset, data, size);
        uploadGpu(offset, data, size);
    }
    bgfx::DynamicIndexBufferHandle getHandle() const { return handle; }

private:
    void uploadGpu(std::size_t byteOffset, const void* data, std::size_t size) {
        if (byteOffset % getElementSize() != 0 || size % getElementSize() != 0)
            throw std::invalid_argument("bgfx index updates must be index aligned");
        bgfx::update(handle, static_cast<std::uint32_t>(byteOffset / getElementSize()),
                     bgfx::copy(data, static_cast<std::uint32_t>(size)));
    }
    std::vector<std::byte> bytes;
    bgfx::DynamicIndexBufferHandle handle = BGFX_INVALID_HANDLE;
    bool locked = false;
};

class GeometryBgfx final : public Geometry {
public:
    GeometryBgfx(Device* device, const shared_ptr<VertexLayout>& layout,
                 const std::vector<shared_ptr<VertexBuffer>>& vertexBuffers,
                 const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
        : Geometry(device, layout, vertexBuffers, indexBuffer, baseVertexIndex) {
        auto* bgfxLayout = dynamic_cast<VertexLayoutBgfx*>(layout.get());
        if (!bgfxLayout) throw std::invalid_argument("geometry has a non-bgfx vertex layout");
        for (std::size_t stream = 0; stream < vertexBuffers.size(); ++stream) {
            auto* buffer = dynamic_cast<VertexBufferBgfx*>(vertexBuffers[stream].get());
            if (!buffer) throw std::invalid_argument("geometry has a non-bgfx vertex buffer");
            buffer->ensure(bgfxLayout->build(static_cast<unsigned int>(stream),
                                             buffer->getElementSize()));
        }
    }
    void bind(unsigned int offset, unsigned int count,
              unsigned int indexRangeBegin, unsigned int indexRangeEnd) const {
        for (std::size_t stream = 0; stream < vertexBuffers.size(); ++stream) {
            auto* buffer = static_cast<VertexBufferBgfx*>(vertexBuffers[stream].get());
            // Indexed scene batches store indices relative to the geometry's
            // base vertex. indexRangeBegin/End only narrow validation and must
            // not be added to the bound buffer start a second time.
            const unsigned int start = indexBuffer ? baseVertexIndex
                                                   : baseVertexIndex + offset;
            const unsigned int available = static_cast<unsigned int>(buffer->getElementCount());
            const unsigned int requested = indexBuffer ? indexRangeEnd : count;
            bgfx::setVertexBuffer(static_cast<std::uint8_t>(stream), buffer->getHandle(),
                                  start, std::min(requested, available - std::min(start, available)));
        }
        if (indexBuffer) {
            auto* buffer = static_cast<IndexBufferBgfx*>(indexBuffer.get());
            bgfx::setIndexBuffer(buffer->getHandle(), offset, count);
        }
    }
};

class RenderbufferBgfx;

class TextureBgfx final : public Texture, public std::enable_shared_from_this<TextureBgfx> {
public:
    TextureBgfx(Device* device, Type type, Format format, unsigned int width,
                unsigned int height, unsigned int depth, unsigned int mipLevels,
                Usage usage)
        : Texture(device, type, format, width, height, depth, mipLevels, usage),
          nativeFormat((format == Format_L8 || format == Format_LA8)
                  ? bgfx::TextureFormat::RGBA8
                  : textureFormat(format)) {
        checkTextureExtent(width, "texture width");
        checkTextureExtent(height, "texture height");
        checkTextureExtent(depth, "texture depth");
        if (type == Type_Cube && width != height)
            throw std::invalid_argument("cube textures require equal width and height");
        if (usage == Usage_Renderbuffer && type == Type_3D)
            throw std::invalid_argument("3D textures cannot be framebuffer attachments");
        std::uint64_t flags = BGFX_TEXTURE_BLIT_DST;
        if (usage == Usage_Renderbuffer) flags |= BGFX_TEXTURE_RT;
        const bool hasMips = mipLevels > 1;
        if (type == Type_2D)
            handle = bgfx::createTexture2D(static_cast<std::uint16_t>(width),
                static_cast<std::uint16_t>(height), hasMips, 1, nativeFormat, flags);
        else if (type == Type_3D)
            handle = bgfx::createTexture3D(static_cast<std::uint16_t>(width),
                static_cast<std::uint16_t>(height), static_cast<std::uint16_t>(depth),
                hasMips, nativeFormat, flags);
        else
            handle = bgfx::createTextureCube(static_cast<std::uint16_t>(width),
                hasMips, 1, nativeFormat, flags);
        if (!bgfx::isValid(handle)) throw std::runtime_error("bgfx texture creation failed");
    }

    ~TextureBgfx() override {
        if (bgfx::isValid(handle)) bgfx::destroy(handle);
    }

    void upload(unsigned int index, unsigned int mip, const TextureRegion& region,
                const void* data, unsigned int size) override {
        validateRegion(index, mip, region);
        const unsigned int required = Texture::getImageSize(format, region.width,
                                                             region.height) * region.depth;
        if (!data || size != required)
            throw std::invalid_argument("texture upload size does not match its region");
        const bgfx::Memory* memory = nullptr;
        std::vector<std::uint8_t> expanded;
        if (format == Format_L8 || format == Format_LA8) {
            const auto* source = static_cast<const std::uint8_t*>(data);
            const unsigned int pixelCount = region.width * region.height * region.depth;
            expanded.resize(static_cast<std::size_t>(pixelCount) * 4U);
            for (unsigned int pixel = 0; pixel < pixelCount; ++pixel) {
                const std::uint8_t luminance = source[pixel * (format == Format_LA8 ? 2U : 1U)];
                const std::uint8_t alpha = format == Format_LA8
                    ? source[pixel * 2U + 1U]
                    : 255U;
                expanded[pixel * 4U + 0U] = luminance;
                expanded[pixel * 4U + 1U] = luminance;
                expanded[pixel * 4U + 2U] = luminance;
                expanded[pixel * 4U + 3U] = alpha;
            }
            memory = bgfx::copy(expanded.data(),
                static_cast<std::uint32_t>(expanded.size()));
        } else {
            memory = bgfx::copy(data, size);
        }
        if (type == Type_2D)
            bgfx::updateTexture2D(handle, 0, static_cast<std::uint8_t>(mip),
                static_cast<std::uint16_t>(region.x), static_cast<std::uint16_t>(region.y),
                static_cast<std::uint16_t>(region.width),
                static_cast<std::uint16_t>(region.height), memory);
        else if (type == Type_3D)
            bgfx::updateTexture3D(handle, static_cast<std::uint8_t>(mip),
                static_cast<std::uint16_t>(region.x), static_cast<std::uint16_t>(region.y),
                static_cast<std::uint16_t>(region.z), static_cast<std::uint16_t>(region.width),
                static_cast<std::uint16_t>(region.height),
                static_cast<std::uint16_t>(region.depth), memory);
        else
            bgfx::updateTextureCube(handle, 0, static_cast<std::uint8_t>(index),
                static_cast<std::uint8_t>(mip), static_cast<std::uint16_t>(region.x),
                static_cast<std::uint16_t>(region.y), static_cast<std::uint16_t>(region.width),
                static_cast<std::uint16_t>(region.height), memory);
    }

    bool download(unsigned int index, unsigned int mip, void* data,
                  unsigned int size) override {
        if (!data || mip >= mipLevels || index >= (type == Type_Cube ? 6U : 1U) ||
            type == Type_3D)
            return false;
        const unsigned int mipWidth = getMipSide(width, mip);
        const unsigned int mipHeight = getMipSide(height, mip);
        const unsigned int required = getImageSize(format, mipWidth, mipHeight);
        if (size != required || mipWidth > UINT16_MAX || mipHeight > UINT16_MAX) return false;
        if (format == Format_L8 || format == Format_LA8) {
            std::vector<std::uint8_t> expanded(
                static_cast<std::size_t>(mipWidth) * mipHeight * 4U);
            if (!readTexture2D(handle, static_cast<std::uint8_t>(mip),
                    static_cast<std::uint16_t>(index), nativeFormat,
                    static_cast<std::uint16_t>(mipWidth),
                    static_cast<std::uint16_t>(mipHeight), expanded.data(),
                    expanded.size()))
                return false;
            auto* destination = static_cast<std::uint8_t*>(data);
            const unsigned int pixelCount = mipWidth * mipHeight;
            for (unsigned int pixel = 0; pixel < pixelCount; ++pixel) {
                destination[pixel * (format == Format_LA8 ? 2U : 1U)] =
                    expanded[pixel * 4U];
                if (format == Format_LA8)
                    destination[pixel * 2U + 1U] = expanded[pixel * 4U + 3U];
            }
            return true;
        }
        return readTexture2D(handle, static_cast<std::uint8_t>(mip),
            static_cast<std::uint16_t>(index), nativeFormat,
            static_cast<std::uint16_t>(mipWidth), static_cast<std::uint16_t>(mipHeight),
            data, size);
    }

    bool supportsLocking() const override { return true; }

    LockResult lock(unsigned int index, unsigned int mip,
                    const TextureRegion& region) override {
        if (lockState.active) throw std::logic_error("texture is already locked");
        validateRegion(index, mip, region);
        lockState.active = true;
        lockState.index = index;
        lockState.mip = mip;
        lockState.region = region;
        lockState.bytes.resize(getImageSize(format, region.width, region.height) * region.depth);
        return {lockState.bytes.data(), getImageSize(format, region.width, 1),
                getImageSize(format, region.width, region.height)};
    }

    void unlock(unsigned int index, unsigned int mip) override {
        if (!lockState.active || lockState.index != index || lockState.mip != mip)
            throw std::logic_error("texture unlock does not match the active lock");
        upload(index, mip, lockState.region, lockState.bytes.data(),
               static_cast<unsigned int>(lockState.bytes.size()));
        lockState = {};
    }

    shared_ptr<Renderbuffer> getRenderbuffer(unsigned int index,
                                              unsigned int mip) override;
    void commitChanges() override {}
    void generateMipmaps() override {
        // bgfx exposes mip generation for render targets through framebuffer
        // resolve flags. FramebufferBgfx requests BGFX_RESOLVE_AUTO_GEN_MIPS, so
        // render-target chains are current by the time this synchronization point
        // is reached. Static and dynamic textures still require supplied mip data.
        if (getUsage() != Usage_Renderbuffer || mipLevels <= 1)
            throw std::runtime_error(
                "bgfx mip generation requires a framebuffer-backed mip chain");
    }
    bgfx::TextureHandle getHandle() const { return handle; }
    bgfx::TextureFormat::Enum getNativeFormat() const { return nativeFormat; }

private:
    void validateRegion(unsigned int index, unsigned int mip,
                        const TextureRegion& region) const {
        if (mip >= mipLevels || index >= (type == Type_Cube ? 6U : 1U))
            throw std::out_of_range("texture face or mip is out of range");
        const unsigned int mipWidth = getMipSide(width, mip);
        const unsigned int mipHeight = getMipSide(height, mip);
        const unsigned int mipDepth = getMipSide(depth, mip);
        if (region.width == 0 || region.height == 0 || region.depth == 0 ||
            region.x + region.width > mipWidth || region.y + region.height > mipHeight ||
            region.z + region.depth > mipDepth)
            throw std::out_of_range("texture region is out of range");
    }
    struct LockState final {
        bool active = false;
        unsigned int index = 0;
        unsigned int mip = 0;
        TextureRegion region{};
        std::vector<std::byte> bytes;
    } lockState;
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
    bgfx::TextureFormat::Enum nativeFormat;
    std::map<std::pair<unsigned int, unsigned int>, std::weak_ptr<Renderbuffer>> renderbuffers;
};

class RenderbufferBgfx final : public Renderbuffer {
public:
    RenderbufferBgfx(Device* device, const shared_ptr<TextureBgfx>& texture,
                     unsigned int face, unsigned int mip)
        : Renderbuffer(device, texture->getFormat(), Texture::getMipSide(texture->getWidth(), mip),
                       Texture::getMipSide(texture->getHeight(), mip), 1),
          owner(texture), handle(texture->getHandle()), face(face), mip(mip) {}

    RenderbufferBgfx(Device* device, Texture::Format format, unsigned int width,
                     unsigned int height, unsigned int samples)
        : Renderbuffer(device, format, width, height, samples) {
        checkTextureExtent(width, "renderbuffer width");
        checkTextureExtent(height, "renderbuffer height");
        std::uint64_t flags = multisampleFlags(samples);
        if (Texture::isFormatDepth(format))
            flags |= BGFX_TEXTURE_RT_WRITE_ONLY;
        else
            flags |= BGFX_TEXTURE_BLIT_DST;
        handle = bgfx::createTexture2D(static_cast<std::uint16_t>(width),
            static_cast<std::uint16_t>(height), false, 1, textureFormat(format), flags);
        ownsHandle = true;
        if (!bgfx::isValid(handle)) throw std::runtime_error("bgfx renderbuffer creation failed");
    }

    ~RenderbufferBgfx() override {
        if (ownsHandle && bgfx::isValid(handle)) bgfx::destroy(handle);
    }
    bgfx::TextureHandle getHandle() const { return handle; }
    unsigned int getFace() const { return face; }
    unsigned int getMip() const { return mip; }
    bool shouldAutoGenerateMips() const {
        return mip == 0 && owner && owner->getMipLevels() > 1;
    }

private:
    shared_ptr<TextureBgfx> owner;
    bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
    unsigned int face = 0;
    unsigned int mip = 0;
    bool ownsHandle = false;
};

shared_ptr<Renderbuffer> TextureBgfx::getRenderbuffer(unsigned int index,
                                                      unsigned int mip) {
    if (usage != Usage_Renderbuffer || mip >= mipLevels ||
        index >= (type == Type_Cube ? 6U : 1U))
        throw std::invalid_argument("texture cannot be used as the requested renderbuffer");
    auto& slot = renderbuffers[{index, mip}];
    shared_ptr<Renderbuffer> result = slot.lock();
    if (!result) {
        result = shared_ptr<Renderbuffer>(new RenderbufferBgfx(
            device, shared_from_this(), index, mip));
        slot = result;
    }
    return result;
}

class FramebufferBgfx final : public Framebuffer {
public:
    FramebufferBgfx(Device* device, unsigned int width, unsigned int height,
                    std::function<void(void*, unsigned int)> downloadMain)
        : Framebuffer(device, width, height, 1), downloadMain(std::move(downloadMain)) {}

    FramebufferBgfx(Device* device, const std::vector<shared_ptr<Renderbuffer>>& color,
                    const shared_ptr<Renderbuffer>& depth)
        : Framebuffer(device, dimensions(color, depth, 0), dimensions(color, depth, 1),
                      dimensions(color, depth, 2)), color(color), depth(depth) {
        if (color.empty() && !depth)
            throw std::invalid_argument("framebuffer requires at least one attachment");
        std::vector<bgfx::Attachment> attachments;
        attachments.reserve(color.size() + (depth ? 1U : 0U));
        for (const auto& resource : color) attachments.push_back(attachment(resource, false));
        if (depth) attachments.push_back(attachment(depth, true));
        const bool attachmentsValid = bgfx::isFrameBufferValid(
            static_cast<std::uint8_t>(attachments.size()), attachments.data());
        handle = bgfx::createFrameBuffer(static_cast<std::uint8_t>(attachments.size()),
                                          attachments.data(), false);
        if (!bgfx::isValid(handle)) {
            std::string detail = "bgfx framebuffer creation failed: " +
                std::to_string(attachments.size()) + " attachment(s), " +
                std::to_string(width) + "x" + std::to_string(height);
            if (!color.empty()) {
                const auto* first = static_cast<const RenderbufferBgfx*>(color.front().get());
                detail += ", face " + std::to_string(first->getFace()) +
                    ", mip " + std::to_string(first->getMip());
            }
            detail += attachmentsValid ? ", validation passed" : ", validation failed";
            for (std::size_t index = 0; index < attachments.size(); ++index) {
                detail += bgfx::isFrameBufferValid(1, &attachments[index])
                    ? ", attachment " + std::to_string(index) + " valid"
                    : ", attachment " + std::to_string(index) + " invalid";
            }
            throw std::runtime_error(detail);
        }
    }
    ~FramebufferBgfx() override { if (bgfx::isValid(handle)) bgfx::destroy(handle); }
    void download(void* data, unsigned int size) override {
        if (downloadMain) {
            downloadMain(data, size);
            return;
        }
        if (color.empty() || size != width * height * 4U)
            throw std::invalid_argument("framebuffer download requires RGBA8-sized output");
        auto* buffer = static_cast<RenderbufferBgfx*>(color.front().get());
        if (buffer->getFormat() != Texture::Format_RGBA8 || buffer->getSamples() != 1 ||
            !readTexture2D(buffer->getHandle(), static_cast<std::uint8_t>(buffer->getMip()),
                static_cast<std::uint16_t>(buffer->getFace()), bgfx::TextureFormat::RGBA8,
                static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height), data, size))
            throw std::runtime_error("framebuffer readback is unsupported for this attachment");
    }
    bgfx::FrameBufferHandle getHandle() const { return handle; }
    void resizeMain(unsigned int newWidth, unsigned int newHeight) {
        if (!downloadMain)
            throw std::logic_error("only the main framebuffer can be resized");
        width = newWidth;
        height = newHeight;
    }
    RenderbufferBgfx* firstColor() const {
        return color.empty() ? nullptr : static_cast<RenderbufferBgfx*>(color.front().get());
    }

private:
    static unsigned int dimensions(const std::vector<shared_ptr<Renderbuffer>>& color,
                                   const shared_ptr<Renderbuffer>& depth, unsigned int axis) {
        const Renderbuffer* first = !color.empty() ? color.front().get() : depth.get();
        if (!first) return 0;
        const unsigned int value = axis == 0 ? first->getWidth()
            : axis == 1 ? first->getHeight() : first->getSamples();
        auto validate = [axis, value](const shared_ptr<Renderbuffer>& item) {
            if (!item) throw std::invalid_argument("framebuffer attachment is null");
            const unsigned int candidate = axis == 0 ? item->getWidth()
                : axis == 1 ? item->getHeight() : item->getSamples();
            if (candidate != value) throw std::invalid_argument("framebuffer attachment dimensions differ");
            if (!dynamic_cast<RenderbufferBgfx*>(item.get()))
                throw std::invalid_argument("framebuffer has a non-bgfx attachment");
        };
        for (const auto& item : color) validate(item);
        if (depth) validate(depth);
        return value;
    }
    static bgfx::Attachment attachment(const shared_ptr<Renderbuffer>& resource,
                                       bool isDepth) {
        auto* value = dynamic_cast<RenderbufferBgfx*>(resource.get());
        if (!value || Texture::isFormatDepth(value->getFormat()) != isDepth)
            throw std::invalid_argument("framebuffer attachment format is invalid");
        bgfx::Attachment result;
        const std::uint8_t resolve = !isDepth && value->shouldAutoGenerateMips()
            ? BGFX_RESOLVE_AUTO_GEN_MIPS
            : BGFX_RESOLVE_NONE;
        result.init(value->getHandle(), bgfx::Access::Write,
                    static_cast<std::uint16_t>(value->getFace()),
                    1, static_cast<std::uint16_t>(value->getMip()), resolve);
        return result;
    }
    std::vector<shared_ptr<Renderbuffer>> color;
    shared_ptr<Renderbuffer> depth;
    std::function<void(void*, unsigned int)> downloadMain;
    bgfx::FrameBufferHandle handle = BGFX_INVALID_HANDLE;
};

class VertexShaderBgfx final : public VertexShader {
public:
    VertexShaderBgfx(Device* device, const std::vector<char>& bytecode)
        : VertexShader(device) { reloadBytecode(bytecode); }
    ~VertexShaderBgfx() override { if (bgfx::isValid(handle)) bgfx::destroy(handle); }
    void reloadBytecode(const std::vector<char>& bytecode) override {
        if (bytecode.empty()) throw std::invalid_argument("vertex shader bytecode is empty");
        bgfx::ShaderHandle replacement = bgfx::createShader(
            bgfx::copy(bytecode.data(), static_cast<std::uint32_t>(bytecode.size())));
        if (!bgfx::isValid(replacement)) throw std::runtime_error("bgfx vertex shader creation failed");
        if (bgfx::isValid(handle)) bgfx::destroy(handle);
        handle = replacement;
    }
    bgfx::ShaderHandle getHandle() const { return handle; }
private:
    bgfx::ShaderHandle handle = BGFX_INVALID_HANDLE;
};

class FragmentShaderBgfx final : public FragmentShader {
public:
    FragmentShaderBgfx(Device* device, const std::vector<char>& bytecode)
        : FragmentShader(device) { reloadBytecode(bytecode); }
    ~FragmentShaderBgfx() override { if (bgfx::isValid(handle)) bgfx::destroy(handle); }
    void reloadBytecode(const std::vector<char>& bytecode) override {
        if (bytecode.empty()) throw std::invalid_argument("fragment shader bytecode is empty");
        bgfx::ShaderHandle replacement = bgfx::createShader(
            bgfx::copy(bytecode.data(), static_cast<std::uint32_t>(bytecode.size())));
        if (!bgfx::isValid(replacement)) throw std::runtime_error("bgfx fragment shader creation failed");
        if (bgfx::isValid(handle)) bgfx::destroy(handle);
        handle = replacement;
    }
    bgfx::ShaderHandle getHandle() const { return handle; }
private:
    bgfx::ShaderHandle handle = BGFX_INVALID_HANDLE;
};

class ShaderProgramBgfx final : public ShaderProgram {
public:
    struct Uniform final {
        bgfx::UniformHandle handle = BGFX_INVALID_HANDLE;
        bgfx::UniformInfo info{};
    };

    ShaderProgramBgfx(Device* device, const shared_ptr<VertexShader>& vertexShader,
                      const shared_ptr<FragmentShader>& fragmentShader)
        : ShaderProgram(device, vertexShader, fragmentShader) {
        auto* vertex = dynamic_cast<VertexShaderBgfx*>(vertexShader.get());
        auto* fragment = dynamic_cast<FragmentShaderBgfx*>(fragmentShader.get());
        if (!vertex || !fragment) throw std::invalid_argument("program has non-bgfx shaders");
        handle = bgfx::createProgram(vertex->getHandle(), fragment->getHandle(), false);
        if (!bgfx::isValid(handle)) throw std::runtime_error("bgfx program link failed");
        collectUniforms(vertex->getHandle());
        collectUniforms(fragment->getHandle());
        for (std::size_t index = 0; index < uniforms.size(); ++index) {
            const std::string_view name(uniforms[index].info.name);
            if (name == "WorldMatrixArray" || name == "u_worldMatrixArray") {
                worldArray = static_cast<int>(index);
                maxWorldTransforms = std::max(1U,
                    static_cast<unsigned int>(uniforms[index].info.num / 3U));
            } else if (name == "WorldMatrix" || name == "u_worldMatrix") {
                worldMatrix = static_cast<int>(index);
                maxWorldTransforms = 1;
            }
            if (uniforms[index].info.type == bgfx::UniformType::Sampler && samplerCount < 32) {
                unsigned int stage = samplerCount;
                if (name.size() > 2 && name[0] == 's') {
                    std::size_t cursor = 1;
                    unsigned int parsed = 0;
                    while (cursor < name.size() && name[cursor] >= '0' && name[cursor] <= '9') {
                        parsed = parsed * 10U + static_cast<unsigned int>(name[cursor] - '0');
                        ++cursor;
                    }
                    if (cursor < name.size() && name[cursor] == '_')
                        stage = parsed;
                }
                if (stage >= samplerByStage.size())
                    throw std::runtime_error("shader sampler stage exceeds bgfx limits");
                samplerByStage[stage] = static_cast<int>(index);
                samplerMask |= 1U << stage;
                ++samplerCount;
            }
        }
    }
    ~ShaderProgramBgfx() override { if (bgfx::isValid(handle)) bgfx::destroy(handle); }
    int getConstantHandle(const char* name) const override {
        for (std::size_t index = 0; index < uniforms.size(); ++index)
            if (std::string_view(uniforms[index].info.name) == name)
                return static_cast<int>(index);
        return -1;
    }
    unsigned int getMaxWorldTransforms() const override { return maxWorldTransforms; }
    unsigned int getSamplerMask() const override { return samplerMask; }
    bgfx::ProgramHandle getHandle() const { return handle; }
    bgfx::UniformHandle sampler(unsigned int stage) const {
        if (stage >= samplerByStage.size() || samplerByStage[stage] < 0)
            return BGFX_INVALID_HANDLE;
        return uniforms[static_cast<std::size_t>(samplerByStage[stage])].handle;
    }
    void setConstant(int constant, const float* data, std::size_t vectorCount) const {
        // Shader compilers may optimize an optional constant out. The legacy
        // GL/D3D backends treat the resulting -1 handle as a deliberate no-op.
        if (constant < 0)
            return;
        if (static_cast<std::size_t>(constant) >= uniforms.size())
            throw std::out_of_range("shader constant handle is invalid");
        const Uniform& uniform = uniforms[static_cast<std::size_t>(constant)];
        std::size_t count = vectorCount;
        if (uniform.info.type == bgfx::UniformType::Mat4) count = vectorCount / 4U;
        if (uniform.info.type == bgfx::UniformType::Mat3) count = vectorCount / 3U;
        if (count == 0 || count > uniform.info.num)
            throw std::out_of_range("shader constant data exceeds its declared capacity");
        bgfx::setUniform(uniform.handle, data, static_cast<std::uint16_t>(count));
    }
    void setGlobalConstants(const std::vector<GlobalConstantBinding>& bindings,
                            const std::vector<std::byte>& data) const {
        for (const GlobalConstantBinding& binding : bindings) {
            const int handle = getConstantHandle(binding.name.c_str());
            if (handle < 0) continue;
            const std::size_t end = binding.offset + binding.size;
            if (end > data.size() || binding.size % (sizeof(float) * 4U) != 0U)
                throw std::runtime_error("global shader constant layout is invalid");
            const Uniform& uniform = uniforms[static_cast<std::size_t>(handle)];
            const float* source = reinterpret_cast<const float*>(data.data() + binding.offset);
            if (uniform.info.type == bgfx::UniformType::Mat4) {
                const std::size_t matrixCount = binding.size / (sizeof(float) * 16U);
                if (matrixCount == 0 || binding.size != matrixCount * sizeof(float) * 16U)
                    throw std::runtime_error("global matrix layout is invalid");
                std::vector<float> transposed(matrixCount * 16U);
                for (std::size_t matrix = 0; matrix < matrixCount; ++matrix) {
                    for (std::size_t row = 0; row < 4U; ++row) {
                        for (std::size_t column = 0; column < 4U; ++column) {
                            transposed[matrix * 16U + column * 4U + row] =
                                source[matrix * 16U + row * 4U + column];
                        }
                    }
                }
                bgfx::setUniform(uniform.handle, transposed.data(),
                    static_cast<std::uint16_t>(matrixCount));
            } else {
                setConstant(handle, source, binding.size / (sizeof(float) * 4U));
            }
        }
    }
    void setWorldTransforms(const float* data, std::size_t matrixCount) const {
        // The inherited UI streamer uses a null/zero call to clear the fixed-
        // function transform path. Shader backends have no corresponding state.
        if (matrixCount == 0)
            return;
        if (!data || matrixCount > maxWorldTransforms)
            throw std::out_of_range("world transform count exceeds shader capacity");
        if (worldArray >= 0) {
            bgfx::setUniform(uniforms[static_cast<std::size_t>(worldArray)].handle, data,
                             static_cast<std::uint16_t>(matrixCount * 3U));
            return;
        }
        float matrix[16] = {
            data[0], data[4], data[8], 0.0F,
            data[1], data[5], data[9], 0.0F,
            data[2], data[6], data[10], 0.0F,
            data[3], data[7], data[11], 1.0F};
        if (worldMatrix >= 0)
            bgfx::setUniform(uniforms[static_cast<std::size_t>(worldMatrix)].handle, matrix);
        else
            bgfx::setTransform(matrix);
    }

private:
    void collectUniforms(bgfx::ShaderHandle shader) {
        const std::uint16_t count = bgfx::getShaderUniforms(shader);
        std::vector<bgfx::UniformHandle> handles(count);
        bgfx::getShaderUniforms(shader, handles.data(), count);
        for (bgfx::UniformHandle uniformHandle : handles) {
            if (std::any_of(uniforms.begin(), uniforms.end(), [uniformHandle](const Uniform& value) {
                    return value.handle.idx == uniformHandle.idx;
                })) continue;
            Uniform uniform;
            uniform.handle = uniformHandle;
            bgfx::getUniformInfo(uniformHandle, uniform.info);
            uniforms.push_back(uniform);
        }
    }
    bgfx::ProgramHandle handle = BGFX_INVALID_HANDLE;
    std::vector<Uniform> uniforms;
    std::array<int, 32> samplerByStage = [] {
        std::array<int, 32> value{};
        value.fill(-1);
        return value;
    }();
    int worldMatrix = -1;
    int worldArray = -1;
    unsigned int maxWorldTransforms = 1;
    unsigned int samplerMask = 0;
    unsigned int samplerCount = 0;
};

class DeviceBgfx::Context final : public DeviceContext {
public:
    explicit Context(FramebufferBgfx* mainFramebuffer)
        : mainFramebuffer(mainFramebuffer), framebuffer(mainFramebuffer) {}
    void prepareFrame() {
        view = 0;
        framebuffer = mainFramebuffer;
        bgfx::setViewFrameBuffer(view, BGFX_INVALID_HANDLE);
        bgfx::setViewRect(view, 0, 0, static_cast<std::uint16_t>(mainFramebuffer->getWidth()),
                          static_cast<std::uint16_t>(mainFramebuffer->getHeight()));
    }
    void setDefaultAnisotropy(unsigned int value) override { anisotropy = value; }
    void defineGlobalConstants(size_t dataSize,
                               const std::vector<ShaderGlobalConstant>& constants) {
        globalBindings.clear();
        globalBindings.reserve(constants.size());
        for (const ShaderGlobalConstant& constant : constants) {
            if (!constant.name || constant.offset + constant.size > dataSize)
                throw std::invalid_argument("global shader constant definition is invalid");
            globalBindings.push_back(
                {constant.name, constant.offset, constant.size});
        }
        globalData.assign(dataSize, std::byte{});
    }
    void updateGlobalConstants(const void* data, size_t dataSize) override {
        if ((!data && dataSize != 0U) || dataSize != globalData.size())
            throw std::invalid_argument("global shader constant payload size is invalid");
        if (dataSize != 0U)
            std::memcpy(globalData.data(), data, dataSize);
    }
    void bindFramebuffer(Framebuffer* value) override {
        if (!value) value = mainFramebuffer;
        auto* replacement = dynamic_cast<FramebufferBgfx*>(value);
        if (value && !replacement)
            throw std::invalid_argument("attempted to bind a non-bgfx framebuffer");
        if (replacement == framebuffer) return;
        if (view == std::numeric_limits<bgfx::ViewId>::max())
            throw std::runtime_error("frame exceeded the bgfx view limit");
        framebuffer = replacement;
        ++view;
        bgfx::FrameBufferHandle handle = BGFX_INVALID_HANDLE;
        if (framebuffer) handle = framebuffer->getHandle();
        bgfx::setViewFrameBuffer(view, handle);
        bgfx::setViewRect(view, 0, 0, static_cast<std::uint16_t>(framebuffer->getWidth()),
                          static_cast<std::uint16_t>(framebuffer->getHeight()));
    }
    void clearFramebuffer(unsigned int mask, const float color[4], float depth,
                          unsigned int stencil) override {
        const auto rgba = (static_cast<std::uint32_t>(color[0] * 255.0F) << 24U) |
                          (static_cast<std::uint32_t>(color[1] * 255.0F) << 16U) |
                          (static_cast<std::uint32_t>(color[2] * 255.0F) << 8U) |
                          static_cast<std::uint32_t>(color[3] * 255.0F);
        std::uint16_t clear = 0;
        if ((mask & Buffer_Color) != 0U) clear |= BGFX_CLEAR_COLOR;
        if ((mask & Buffer_Depth) != 0U) clear |= BGFX_CLEAR_DEPTH;
        if ((mask & Buffer_Stencil) != 0U) clear |= BGFX_CLEAR_STENCIL;
        bgfx::setViewClear(view, clear, rgba, depth,
                           static_cast<std::uint8_t>(stencil));
        bgfx::touch(view);
    }
    void copyFramebuffer(Framebuffer* source, Texture* destination) override {
        auto* sourceValue = dynamic_cast<FramebufferBgfx*>(source);
        auto* destinationValue = dynamic_cast<TextureBgfx*>(destination);
        RenderbufferBgfx* color = sourceValue ? sourceValue->firstColor() : nullptr;
        if (!color || !destinationValue || destinationValue->getType() != Texture::Type_2D ||
            sourceValue->getWidth() != destinationValue->getWidth() ||
            sourceValue->getHeight() != destinationValue->getHeight())
            throw std::invalid_argument("framebuffer copy resources are incompatible");
        bgfx::blit(view, destinationValue->getHandle(), 0, 0, color->getHandle(), 0, 0,
                   static_cast<std::uint16_t>(sourceValue->getWidth()),
                   static_cast<std::uint16_t>(sourceValue->getHeight()));
    }
    void resolveFramebuffer(Framebuffer* source, Framebuffer* destination,
                            unsigned int mask) override {
        auto* sourceValue = dynamic_cast<FramebufferBgfx*>(source);
        auto* destinationValue = dynamic_cast<FramebufferBgfx*>(destination);
        RenderbufferBgfx* sourceColor = sourceValue ? sourceValue->firstColor() : nullptr;
        RenderbufferBgfx* destinationColor = destinationValue ? destinationValue->firstColor() : nullptr;
        if ((mask & Buffer_Color) == 0U || !sourceColor || !destinationColor ||
            sourceValue->getWidth() != destinationValue->getWidth() ||
            sourceValue->getHeight() != destinationValue->getHeight())
            throw std::invalid_argument("framebuffer resolve resources are incompatible");
        bgfx::blit(view, destinationColor->getHandle(), 0, 0, sourceColor->getHandle(), 0, 0,
                   static_cast<std::uint16_t>(sourceValue->getWidth()),
                   static_cast<std::uint16_t>(sourceValue->getHeight()));
    }
    void discardFramebuffer(Framebuffer*, unsigned int) override {}
    void bindProgram(ShaderProgram* value) override {
        program = dynamic_cast<ShaderProgramBgfx*>(value);
        if (value && !program) throw std::invalid_argument("attempted to bind a non-bgfx program");
    }
    void setWorldTransforms4x3(const float* data, size_t matrixCount) override {
        requireProgram().setWorldTransforms(data, matrixCount);
    }
    void setConstant(int handle, const float* data, size_t vectorCount) override {
        requireProgram().setConstant(handle, data, vectorCount);
    }
    void bindTexture(unsigned int stage, Texture* texture,
                     const SamplerState& sampler) override {
        if (stage >= 32) throw std::out_of_range("texture stage exceeds GfxCore limit");
        auto* value = dynamic_cast<TextureBgfx*>(texture);
        if (texture && !value) throw std::invalid_argument("attempted to bind a non-bgfx texture");
        const bgfx::UniformHandle uniform = requireProgram().sampler(stage);
        // The shared render stream binds its default texture for both textured
        // and solid-color UI programs.  Solid programs legitimately optimize
        // the sampler away, so there is no binding to submit for that stage.
        if (!bgfx::isValid(uniform)) return;
        std::uint32_t flags = 0;
        if (sampler.getFilter() == SamplerState::Filter_Point)
            flags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
        else if (sampler.getFilter() == SamplerState::Filter_Anisotropic)
            flags |= BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC;
        if (sampler.getAddress() == SamplerState::Address_Clamp)
            flags |= BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP | BGFX_SAMPLER_W_CLAMP;
        bgfx::TextureHandle handle = BGFX_INVALID_HANDLE;
        if (value) handle = value->getHandle();
        bgfx::setTexture(static_cast<std::uint8_t>(stage), uniform, handle, flags);
    }
    void setRasterizerState(const RasterizerState& value) override {
        state &= ~(BGFX_STATE_CULL_CW | BGFX_STATE_CULL_CCW);
        // The scene geometry follows the OpenGL convention: counter-clockwise
        // triangles are front-facing, so back-face culling removes clockwise
        // winding. bgfx cull flags name the winding that is removed.
        if (value.getCullMode() == RasterizerState::Cull_Back) state |= BGFX_STATE_CULL_CW;
        if (value.getCullMode() == RasterizerState::Cull_Front) state |= BGFX_STATE_CULL_CCW;
    }
    void setBlendState(const BlendState& value) override {
        state &= ~(BGFX_STATE_BLEND_MASK | BGFX_STATE_WRITE_R | BGFX_STATE_WRITE_G |
                   BGFX_STATE_WRITE_B | BGFX_STATE_WRITE_A);
        const unsigned int mask = value.getColorMask();
        if ((mask & BlendState::Color_R) != 0U) state |= BGFX_STATE_WRITE_R;
        if ((mask & BlendState::Color_G) != 0U) state |= BGFX_STATE_WRITE_G;
        if ((mask & BlendState::Color_B) != 0U) state |= BGFX_STATE_WRITE_B;
        if ((mask & BlendState::Color_A) != 0U) state |= BGFX_STATE_WRITE_A;
        if (value.blendingNeeded()) {
            state |= BGFX_STATE_BLEND_FUNC_SEPARATE(
                blendFactor(static_cast<BlendState::Factor>(value.getColorSrc())),
                blendFactor(static_cast<BlendState::Factor>(value.getColorDst())),
                blendFactor(static_cast<BlendState::Factor>(value.getAlphaSrc())),
                blendFactor(static_cast<BlendState::Factor>(value.getAlphaDst())));
        }
    }
    void setDepthState(const DepthState& value) override {
        state &= ~(BGFX_STATE_DEPTH_TEST_MASK | BGFX_STATE_WRITE_Z);
        if (value.getFunction() == DepthState::Function_Less) state |= BGFX_STATE_DEPTH_TEST_LESS;
        if (value.getFunction() == DepthState::Function_LessEqual) state |= BGFX_STATE_DEPTH_TEST_LEQUAL;
        if (value.getFunction() == DepthState::Function_Always) state |= BGFX_STATE_DEPTH_TEST_ALWAYS;
        if (value.getWrite()) state |= BGFX_STATE_WRITE_Z;
        if (value.getStencilMode() == DepthState::Stencil_IsNotZero)
            bgfx::setStencil(BGFX_STENCIL_TEST_NOTEQUAL | BGFX_STENCIL_FUNC_REF(0));
        else if (value.getStencilMode() == DepthState::Stencil_UpdateZFail)
            bgfx::setStencil(BGFX_STENCIL_TEST_ALWAYS | BGFX_STENCIL_OP_FAIL_S_KEEP |
                             BGFX_STENCIL_OP_FAIL_Z_INCR | BGFX_STENCIL_OP_PASS_Z_KEEP);
        else
            bgfx::setStencil(BGFX_STENCIL_NONE);
    }
    void setScissor(unsigned int x, unsigned int y,
                    unsigned int width, unsigned int height) override {
        if (width == 0 || height == 0 || x > UINT16_MAX || y > UINT16_MAX ||
            width > UINT16_MAX || height > UINT16_MAX)
            throw std::invalid_argument("scissor rectangle is invalid");
        scissor = {static_cast<std::uint16_t>(x), static_cast<std::uint16_t>(y),
                   static_cast<std::uint16_t>(width), static_cast<std::uint16_t>(height)};
        scissorEnabled = true;
    }
    void clearScissor() override { scissorEnabled = false; }
    bool drawTransient(VertexLayout* layout, Geometry::Primitive primitive,
        const void* vertexData, unsigned int vertexStride, unsigned int vertexCount,
        const void* indexData, unsigned int indexElementSize,
        unsigned int indexCount) override {
        auto* value = dynamic_cast<VertexLayoutBgfx*>(layout);
        if (!value || !vertexData || vertexStride == 0 || vertexCount == 0)
            throw std::invalid_argument("transient geometry description is invalid");
        for (const VertexLayout::Element& element : value->getElements())
            if (element.stream != 0)
                throw std::invalid_argument("transient geometry supports one vertex stream");
        const bool indexed = indexData != nullptr || indexCount != 0 || indexElementSize != 0;
        if (indexed && (!indexData || indexCount == 0 ||
                        (indexElementSize != 2 && indexElementSize != 4)))
            throw std::invalid_argument("transient index data is invalid");
        const bgfx::VertexLayout nativeLayout = value->build(0, vertexStride);
        if (bgfx::getAvailTransientVertexBuffer(vertexCount, nativeLayout) != vertexCount ||
            (indexed && bgfx::getAvailTransientIndexBuffer(
                indexCount, indexElementSize == 4) != indexCount))
            return false;
        bgfx::TransientVertexBuffer vertices{};
        bgfx::allocTransientVertexBuffer(&vertices, vertexCount, nativeLayout);
        std::memcpy(vertices.data, vertexData,
                    static_cast<std::size_t>(vertexStride) * vertexCount);
        bgfx::setVertexBuffer(0, &vertices, 0, vertexCount);
        if (indexed) {
            bgfx::TransientIndexBuffer indices{};
            bgfx::allocTransientIndexBuffer(&indices, indexCount, indexElementSize == 4);
            std::memcpy(indices.data, indexData,
                        static_cast<std::size_t>(indexElementSize) * indexCount);
            bgfx::setIndexBuffer(&indices, 0, indexCount);
        }
        submit(primitive);
        return true;
    }
    void pushDebugMarkerGroup(const char* text) override { bgfx::setMarker(text); }
    void popDebugMarkerGroup() override {}
    void setDebugMarker(const char* text) override { bgfx::setMarker(text); }

protected:
    void drawImpl(Geometry* geometry, Geometry::Primitive primitive,
                  unsigned int offset, unsigned int count,
                  unsigned int indexRangeBegin, unsigned int indexRangeEnd) override {
        auto* value = dynamic_cast<GeometryBgfx*>(geometry);
        if (!value) throw std::invalid_argument("attempted to draw non-bgfx geometry");
        value->bind(offset, count, indexRangeBegin, indexRangeEnd);
        submit(primitive);
    }

private:
    static std::uint64_t blendFactor(BlendState::Factor value) {
        switch (value) {
        case BlendState::Factor_One: return BGFX_STATE_BLEND_ONE;
        case BlendState::Factor_Zero: return BGFX_STATE_BLEND_ZERO;
        case BlendState::Factor_DstColor: return BGFX_STATE_BLEND_DST_COLOR;
        case BlendState::Factor_SrcAlpha: return BGFX_STATE_BLEND_SRC_ALPHA;
        case BlendState::Factor_InvSrcAlpha: return BGFX_STATE_BLEND_INV_SRC_ALPHA;
        case BlendState::Factor_DstAlpha: return BGFX_STATE_BLEND_DST_ALPHA;
        case BlendState::Factor_InvDstAlpha: return BGFX_STATE_BLEND_INV_DST_ALPHA;
        default: throw std::invalid_argument("unsupported blend factor");
        }
    }
    ShaderProgramBgfx& requireProgram() const {
        if (!program) throw std::logic_error("draw or uniform update requires a bound program");
        return *program;
    }
    void submit(Geometry::Primitive primitive) {
        requireProgram().setGlobalConstants(globalBindings, globalData);
        if (scissorEnabled)
            bgfx::setScissor(scissor[0], scissor[1], scissor[2], scissor[3]);
        else
            bgfx::setScissor(UINT16_MAX);
        std::uint64_t drawState = state;
        if (primitive == Geometry::Primitive_Lines) drawState |= BGFX_STATE_PT_LINES;
        if (primitive == Geometry::Primitive_Points) drawState |= BGFX_STATE_PT_POINTS;
        if (primitive == Geometry::Primitive_TriangleStrip) drawState |= BGFX_STATE_PT_TRISTRIP;
        bgfx::setState(drawState);
        bgfx::submit(view, requireProgram().getHandle());
    }
    unsigned int anisotropy = 1;
    bgfx::ViewId view = 0;
    FramebufferBgfx* mainFramebuffer = nullptr;
    FramebufferBgfx* framebuffer = nullptr;
    ShaderProgramBgfx* program = nullptr;
    std::array<std::uint16_t, 4> scissor{};
    std::vector<GlobalConstantBinding> globalBindings;
    std::vector<std::byte> globalData;
    bool scissorEnabled = false;
    std::uint64_t state = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A |
                          BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS |
                          BGFX_STATE_CULL_CCW | BGFX_STATE_MSAA;
};

DeviceBgfx::DeviceBgfx(const DeviceWindow& window)
    : callback(std::make_unique<Callback>()), width(window.width), height(window.height),
      resetFlags(BGFX_RESET_VSYNC) {
    checkTextureExtent(width, "window width");
    checkTextureExtent(height, "window height");
    if (!window.windowHandle)
        throw std::invalid_argument("bgfx requires a native window handle");
    bgfx::Init init{};
    init.type = rendererType(window.renderer);
    init.platformData.nwh = window.windowHandle;
    init.platformData.ndt = window.displayHandle;
    init.platformData.context = window.graphicsContext;
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = resetFlags;
    init.callback = callback.get();
    if (!bgfx::init(init)) {
        throw std::runtime_error("bgfx failed to initialize through GfxCore");
    }
    initialized = true;
    mainFramebuffer = shared_ptr<Framebuffer>(new FramebufferBgfx(
        this, width, height, [this](void* data, unsigned int size) {
            callback->captureMainFramebuffer(width, height, data, size);
        }));
    context = std::make_unique<Context>(static_cast<FramebufferBgfx*>(mainFramebuffer.get()));
    const bgfx::Caps* source = bgfx::getCaps();
    caps.supportsFramebuffer = true;
    caps.supportsShaders = true;
    caps.supportsFFP = false;
    caps.supportsStencil = true;
    caps.supportsIndex32 = (source->supported & BGFX_CAPS_INDEX32) != 0U;
    caps.supportsTextureDXT = (source->formats[bgfx::TextureFormat::BC1] &
                               BGFX_CAPS_FORMAT_TEXTURE_2D) != 0U;
    caps.supportsTexturePVR = (source->formats[bgfx::TextureFormat::PTC14] &
                               BGFX_CAPS_FORMAT_TEXTURE_2D) != 0U;
    caps.supportsTextureHalfFloat = (source->formats[bgfx::TextureFormat::RGBA16F] &
                                     BGFX_CAPS_FORMAT_TEXTURE_2D) != 0U;
    caps.supportsTexture3D = (source->supported & BGFX_CAPS_TEXTURE_3D) != 0U;
    caps.supportsTextureNPOT = true;
    caps.supportsTextureETC1 = (source->formats[bgfx::TextureFormat::ETC1] &
                                BGFX_CAPS_FORMAT_TEXTURE_2D) != 0U;
    caps.supportsTexturePartialMipChain = true;
    caps.maxDrawBuffers = source->limits.maxFBAttachments;
    caps.maxSamples = 16;
    caps.maxTextureSize = source->limits.maxTextureSize;
    caps.maxTextureUnits = source->limits.maxTextureSamplers;
    caps.colorOrderBGR = false;
    caps.needsHalfPixelOffset = false;
    caps.requiresRenderTargetFlipping = source->originBottomLeft;
    caps.retina = window.pixelDensity > 1.0F;
}

DeviceBgfx::~DeviceBgfx() {
    context.reset();
    mainFramebuffer.reset();
    if (initialized) {
        bgfx::frame();
        bgfx::frame();
        bgfx::shutdown();
    }
    callback.reset();
}

bool DeviceBgfx::validate() { return initialized && !suspended && bgfx::getCaps() != nullptr; }
DeviceContext* DeviceBgfx::beginFrame() {
    if (suspended) throw std::logic_error("cannot begin a frame while the device is suspended");
    if (frameActive) throw std::logic_error("a GfxCore frame is already active");
    context->prepareFrame();
    frameActive = true;
    return context.get();
}
void DeviceBgfx::endFrame() {
    if (!frameActive) throw std::logic_error("no GfxCore frame is active");
    bgfx::frame();
    frameActive = false;
}
void DeviceBgfx::resize(unsigned int newWidth, unsigned int newHeight,
                        float pixelDensity) {
    if (frameActive) throw std::logic_error("cannot resize during a GfxCore frame");
    checkTextureExtent(newWidth, "window width");
    checkTextureExtent(newHeight, "window height");
    if (!(pixelDensity > 0.0F) || !std::isfinite(pixelDensity))
        throw std::invalid_argument("pixel density must be finite and positive");
    if (newWidth == width && newHeight == height && caps.retina == (pixelDensity > 1.0F))
        return;
    fireDeviceLost();
    width = newWidth;
    height = newHeight;
    caps.retina = pixelDensity > 1.0F;
    bgfx::reset(width, height, resetFlags);
    static_cast<FramebufferBgfx*>(mainFramebuffer.get())->resizeMain(width, height);
    fireDeviceRestored();
}
Framebuffer* DeviceBgfx::getMainFramebuffer() { return mainFramebuffer.get(); }
DeviceVR* DeviceBgfx::getVR() { return nullptr; }
void DeviceBgfx::setVR(bool enabled) {
    if (enabled) notMigrated("VR");
}
void DeviceBgfx::defineGlobalConstants(size_t dataSize,
    const std::vector<ShaderGlobalConstant>& constants) {
    context->defineGlobalConstants(dataSize, constants);
}
std::string DeviceBgfx::getAPIName() { return "bgfx"; }
std::string DeviceBgfx::getFeatureLevel() { return bgfx::getRendererName(bgfx::getRendererType()); }
std::string DeviceBgfx::getShadingLanguage() {
    switch (bgfx::getRendererType()) {
    case bgfx::RendererType::Direct3D11: return "bgfx-d3d11";
    case bgfx::RendererType::Metal: return "bgfx-metal";
    case bgfx::RendererType::OpenGL: return "bgfx-glsl";
    case bgfx::RendererType::OpenGLES: return "bgfx-gles";
    case bgfx::RendererType::Vulkan: return "bgfx-spirv";
    default: return "bgfx-unknown";
    }
}
std::string DeviceBgfx::createShaderSource(const std::string& path,
    const std::string&, std::function<std::string(const std::string&)> fileCallback) {
    return fileCallback(path);
}
std::vector<char> DeviceBgfx::createShaderBytecode(const std::string&,
    const std::string&, const std::string&) {
    notMigrated("runtime shader compilation is replaced by the shaderc build pipeline");
}
shared_ptr<VertexShader> DeviceBgfx::createVertexShader(const std::vector<char>& bytecode) {
    return shared_ptr<VertexShader>(new VertexShaderBgfx(this, bytecode));
}
shared_ptr<FragmentShader> DeviceBgfx::createFragmentShader(const std::vector<char>& bytecode) {
    return shared_ptr<FragmentShader>(new FragmentShaderBgfx(this, bytecode));
}
shared_ptr<ShaderProgram> DeviceBgfx::createShaderProgram(
    const shared_ptr<VertexShader>& vertexShader,
    const shared_ptr<FragmentShader>& fragmentShader) {
    return shared_ptr<ShaderProgram>(new ShaderProgramBgfx(this, vertexShader, fragmentShader));
}
shared_ptr<ShaderProgram> DeviceBgfx::createShaderProgramFFP() {
    notMigrated("fixed-function programs are unsupported by design");
}
shared_ptr<VertexBuffer> DeviceBgfx::createVertexBuffer(size_t elementSize,
    size_t elementCount, GeometryBuffer::Usage usage) {
    if (elementSize == 0 || elementCount == 0)
        throw std::invalid_argument("vertex buffer dimensions must be non-zero");
    return shared_ptr<VertexBuffer>(new VertexBufferBgfx(this, elementSize, elementCount, usage));
}
shared_ptr<IndexBuffer> DeviceBgfx::createIndexBuffer(size_t elementSize,
    size_t elementCount, GeometryBuffer::Usage usage) {
    if (elementCount == 0) throw std::invalid_argument("index buffer must not be empty");
    return shared_ptr<IndexBuffer>(new IndexBufferBgfx(this, elementSize, elementCount, usage));
}
shared_ptr<VertexLayout> DeviceBgfx::createVertexLayout(
    const std::vector<VertexLayout::Element>& elements) {
    if (elements.empty()) throw std::invalid_argument("vertex layout must not be empty");
    return shared_ptr<VertexLayout>(new VertexLayoutBgfx(this, elements));
}
shared_ptr<Texture> DeviceBgfx::createTexture(Texture::Type type, Texture::Format format,
    unsigned int width, unsigned int height, unsigned int depth,
    unsigned int mipLevels, Texture::Usage usage) {
    if (format >= Texture::Format_Count || mipLevels == 0 ||
        mipLevels > Texture::getMaxMipCount(width, height, depth))
        throw std::invalid_argument("texture description is invalid");
    return shared_ptr<Texture>(new TextureBgfx(this, type, format, width, height,
                                                depth, mipLevels, usage));
}
shared_ptr<Renderbuffer> DeviceBgfx::createRenderbuffer(Texture::Format format,
    unsigned int width, unsigned int height, unsigned int samples) {
    if (format >= Texture::Format_Count || samples == 0 || samples > caps.maxSamples)
        throw std::invalid_argument("renderbuffer description is invalid");
    return shared_ptr<Renderbuffer>(new RenderbufferBgfx(this, format, width, height, samples));
}
const DeviceCaps& DeviceBgfx::getCaps() const { return caps; }
DeviceStats DeviceBgfx::getStatistics() const {
    const bgfx::Stats* stats = bgfx::getStats();
    const double frequency = static_cast<double>(stats->gpuTimerFreq);
    const float milliseconds = frequency > 0.0
        ? static_cast<float>(1000.0 * static_cast<double>(stats->gpuTimeEnd - stats->gpuTimeBegin) / frequency)
        : 0.0F;
    return DeviceStats{milliseconds, stats->numDraw};
}
void DeviceBgfx::suspend() {
    if (suspended) return;
    if (frameActive) throw std::logic_error("cannot suspend during a GfxCore frame");
    fireDeviceLost();
    suspended = true;
}
void DeviceBgfx::resume() {
    if (!suspended) return;
    bgfx::reset(width, height, resetFlags);
    suspended = false;
    fireDeviceRestored();
}
shared_ptr<Geometry> DeviceBgfx::createGeometryImpl(const shared_ptr<VertexLayout>& layout,
    const std::vector<shared_ptr<VertexBuffer>>& vertexBuffers,
    const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex) {
    if (!layout || vertexBuffers.empty())
        throw std::invalid_argument("geometry requires a layout and vertex buffer");
    return shared_ptr<Geometry>(new GeometryBgfx(this, layout, vertexBuffers,
                                                  indexBuffer, baseVertexIndex));
}
shared_ptr<Framebuffer> DeviceBgfx::createFramebufferImpl(
    const std::vector<shared_ptr<Renderbuffer>>& color,
    const shared_ptr<Renderbuffer>& depth) {
    if (color.size() > caps.maxDrawBuffers)
        throw std::invalid_argument("framebuffer exceeds the supported color attachment count");
    return shared_ptr<Framebuffer>(new FramebufferBgfx(this, color, depth));
}

} // namespace RBX::Graphics
