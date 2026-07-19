#include "bgfx/DeviceBgfx.h"
#include <stdexcept>
#ifdef RBX_ENABLE_LEGACY_RENDERERS
#include "GL/DeviceGL.h"
#endif

#if defined(_WIN32) && defined(RBX_ENABLE_LEGACY_RENDERERS)
#include "D3D9/DeviceD3D9.h"
#include "D3D11/DeviceD3D11.h"
#endif

namespace RBX
{
namespace Graphics
{

Device* Device::create(API api, void* windowHandle)
{
    DeviceWindow window;
    window.windowHandle = windowHandle;
    return create(api, window);
}

Device* Device::create(API api, const DeviceWindow& window)
{
    if (api == API_Bgfx)
        return new DeviceBgfx(window);
#if defined(_WIN32) && defined(RBX_ENABLE_LEGACY_RENDERERS)
#if !defined(RBX_PLATFORM_DURANGO)
	if (api == API_Direct3D9)
        return new DeviceD3D9(window.windowHandle);
#endif

    if (api == API_Direct3D11)
        return new DeviceD3D11(window.windowHandle);
#endif

#if !defined(RBX_PLATFORM_DURANGO) && defined(RBX_ENABLE_LEGACY_RENDERERS)
	if (api == API_OpenGL)
        return new DeviceGL(window.windowHandle);
#endif

    throw std::runtime_error("Unsupported GfxCore API");
}

}
}
