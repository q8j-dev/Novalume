#include "GfxCore/Resource.h"

#include "GfxCore/Device.h"

#include <cassert>

namespace RBX
{
namespace Graphics
{

Resource::Resource(Device* device)
    : device(device)
    , prev(NULL)
	, next(NULL)
{
	if (device->resourceListHead)
	{
		assert(device->resourceListTail);

		prev = device->resourceListTail;
		device->resourceListTail->next = this;
		device->resourceListTail = this;
	}
	else
	{
		assert(!device->resourceListTail);

		device->resourceListHead = this;
		device->resourceListTail = this;
	}
}

Resource::~Resource()
{
    if (prev)
		prev->next = next;
	else
	{
		assert(device->resourceListHead == this);
		device->resourceListHead = next;
	}

    if (next)
		next->prev = prev;
	else
	{
		assert(device->resourceListTail == this);
		device->resourceListTail = prev;
	}
}

void Resource::onDeviceLost()
{
}

void Resource::onDeviceRestored()
{
}

void Resource::setDebugName(const std::string& value)
{
    debugName = value;
}

}
}
