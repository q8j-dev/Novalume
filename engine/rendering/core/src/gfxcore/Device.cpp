#include "GfxCore/Device.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <typeinfo>

namespace RBX
{
namespace Graphics
{

DeviceContext::DeviceContext()
{
}

DeviceContext::~DeviceContext()
{
}

void DeviceContext::setScissor(unsigned int, unsigned int, unsigned int, unsigned int)
{
	throw std::runtime_error("scissor rectangles are unsupported by this renderer");
}

void DeviceContext::clearScissor()
{
	throw std::runtime_error("scissor rectangles are unsupported by this renderer");
}

bool DeviceContext::drawTransient(VertexLayout*, Geometry::Primitive,
	const void*, unsigned int, unsigned int, const void*, unsigned int, unsigned int)
{
	throw std::runtime_error("transient geometry is unsupported by this renderer");
}

void DeviceContext::draw(Geometry* geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd)
{
	drawImpl(geometry, primitive, offset, count, indexRangeBegin, indexRangeEnd);
}

void DeviceContext::draw(const GeometryBatch& geometryBatch)
{
	drawImpl(geometryBatch.getGeometry(), geometryBatch.getPrimitive(), geometryBatch.getOffset(), geometryBatch.getCount(), geometryBatch.getIndexRangeBegin(), geometryBatch.getIndexRangeEnd());
}

void DeviceCaps::dumpToFLog(int channel) const
{
    (void)channel;
    std::clog << "GfxCore caps: shaders=" << supportsShaders
              << " framebuffer=" << supportsFramebuffer
              << " maxTexture=" << maxTextureSize
              << " textureUnits=" << maxTextureUnits
              << " retina=" << retina << '\n';
}

DeviceVR::~DeviceVR()
{
}

Device::Device()
	: resourceListHead(NULL)
	, resourceListTail(NULL)
{
}

Device::~Device()
{
	if (resourceListHead)
	{
		std::cerr << "GfxCore: resources remain at device destruction\n";

        unsigned int index = 0;

		for (Resource* cur = resourceListHead; cur; cur = cur->next, index++)
		{
			std::cerr << "  leak " << index << ": " << typeid(*cur).name();

			if (!cur->getDebugName().empty())
				std::cerr << " name=" << cur->getDebugName();
			std::cerr << '\n';
		}
		std::abort();
	}
}

void Device::resize(unsigned int, unsigned int, float)
{
	throw std::runtime_error("surface resize is unsupported by this renderer");
}

shared_ptr<Geometry> Device::createGeometry(const shared_ptr<VertexLayout>& layout, const shared_ptr<VertexBuffer>& vertexBuffer, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
{
    std::vector<shared_ptr<VertexBuffer> > vertexBuffers;
    vertexBuffers.push_back(vertexBuffer);

	return createGeometryImpl(layout, vertexBuffers, indexBuffer, baseVertexIndex);
}

shared_ptr<Geometry> Device::createGeometry(const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
{
	return createGeometryImpl(layout, vertexBuffers, indexBuffer, baseVertexIndex);
}

shared_ptr<Framebuffer> Device::createFramebuffer(const shared_ptr<Renderbuffer>& color, const shared_ptr<Renderbuffer>& depth)
{
    std::vector<shared_ptr<Renderbuffer> > colors;
	colors.push_back(color);

	return createFramebufferImpl(colors, depth);
}

shared_ptr<Framebuffer> Device::createFramebuffer(const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth)
{
	return createFramebufferImpl(color, depth);
}

void Device::fireDeviceLost()
{
    for (Resource* cur = resourceListTail; cur; cur = cur->prev)
		cur->onDeviceLost();
}

void Device::fireDeviceRestored()
{
    for (Resource* cur = resourceListHead; cur; cur = cur->next)
		cur->onDeviceRestored();
}

}
}
