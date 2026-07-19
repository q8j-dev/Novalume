#pragma once

#include "GfxBase/ViewportTextureProvider.h"
#include "TextureRef.h"
#include "rbx/Boost.hpp"
#include <map>
#include <string>

namespace RBX {
class Instance;
namespace Graphics {

class DeviceContext;
class VisualEngine;

class ViewportRenderer
{
public:
    explicit ViewportRenderer(VisualEngine* visualEngine);
    ~ViewportRenderer();

    TextureRef request(const ViewportTextureRequest& request);
    void render(DeviceContext* context);
    void clear();

private:
    struct Job;
    typedef std::pair<Instance*, std::string> JobKey;
    VisualEngine* visualEngine;
    std::map<JobKey, boost::shared_ptr<Job> > jobs;
    unsigned long frameNumber;
};

} // namespace Graphics
} // namespace RBX
