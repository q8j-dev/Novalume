#include "GfxBase/RenderSettings.h"
#include "GfxCore/Device.h"
#include "GfxRender/VisualEngine.h"
#include "rbx/platform/MacHost.h"
#include "v8datamodel/ContentProvider.h"

#include <memory>

int main() {
    RBX::ContentProvider::setAssetFolder(RBX_TEST_VISUAL_ENGINE_RESOURCES "/content");
    auto host = rbx::platform::createMacHost(320, 180, false);
    const auto surface = host->nativeSurface();
    const RBX::Graphics::DeviceWindow window{
        .windowHandle = reinterpret_cast<void*>(surface.window),
        .displayHandle = reinterpret_cast<void*>(surface.display),
        .graphicsContext = reinterpret_cast<void*>(surface.graphicsContext),
        .width = surface.width,
        .height = surface.height,
        .pixelDensity = surface.pixelDensity,
        .renderer = RBX::Graphics::DeviceWindow::Renderer::Metal};
    std::unique_ptr<RBX::Graphics::Device> device(
        RBX::Graphics::Device::create(RBX::Graphics::Device::API_Bgfx, window));
    RBX::CRenderSettings settings;
    RBX::Graphics::VisualEngine visualEngine(device.get(), &settings);
    visualEngine.setViewport(static_cast<int>(surface.width),
                             static_cast<int>(surface.height));
    RBX::Graphics::DeviceContext* context = device->beginFrame();
    const float clearColor[] = {0.02F, 0.03F, 0.05F, 1.0F};
    context->bindFramebuffer(device->getMainFramebuffer());
    context->clearFramebuffer(
        RBX::Graphics::DeviceContext::Buffer_Color |
            RBX::Graphics::DeviceContext::Buffer_Depth,
        clearColor, 1.0F, 0);
    device->endFrame();
    return 0;
}
