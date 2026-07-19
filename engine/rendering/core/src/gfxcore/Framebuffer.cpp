#include "GfxCore/Framebuffer.h"


namespace RBX
{
namespace Graphics
{

Renderbuffer::Renderbuffer(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
	: Resource(device)
    , format(format)
    , width(width)
    , height(height)
    , samples(samples)
{
}

Renderbuffer::~Renderbuffer()
{
}

Framebuffer::Framebuffer(Device* device, unsigned int width, unsigned int height, unsigned int samples)
	: Resource(device)
    , width(width)
    , height(height)
    , samples(samples)
{
}

Framebuffer::~Framebuffer()
{
}

}
}
