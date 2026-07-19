#pragma once

#include <cassert>
#include <memory>
#include <string>

namespace RBX
{
namespace Graphics
{

template<class T>
using shared_ptr = std::shared_ptr<T>;

class Device;

class Resource
{
    friend class Device;

public:
    explicit Resource(Device* device);
    virtual ~Resource();

    virtual void onDeviceLost();
    virtual void onDeviceRestored();

	const std::string& getDebugName() const { return debugName; }
    void setDebugName(const std::string& value);

protected:
    Device* device;

    Resource* prev;
    Resource* next;

    std::string debugName;

private:
    Resource(const Resource&) = delete;
    Resource& operator=(const Resource&) = delete;
};

}
}
