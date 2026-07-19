#include "ViewportRenderer.h"

#include "VisualEngine.h"
#include "SceneManager.h"
#include "SceneUpdater.h"
#include "RenderCamera.h"

#include "GfxCore/Device.h"
#include "GfxCore/Framebuffer.h"
#include "GfxCore/Texture.h"

#include "V8DataModel/Camera.h"
#include "V8DataModel/DataModel.h"

namespace RBX {
namespace Graphics {

struct ViewportRenderer::Job
{
    boost::shared_ptr<Instance> world;
    scoped_ptr<SceneUpdater> updater;
    shared_ptr<Texture> texture;
    shared_ptr<Framebuffer> framebuffer;
    TextureRef textureRef;
    CoordinateFrame cameraCFrame;
    float fieldOfView;
    Color3 ambient;
    Color3 lightColor;
    Vector3 lightDirection;
    unsigned width;
    unsigned height;
    unsigned lastRequestedFrame;
    bool requested;

    Job()
        : fieldOfView(70.0f)
        , ambient(Color3(0.5f, 0.5f, 0.5f))
        , lightColor(Color3::white())
        , lightDirection(-1.0f, -1.0f, -1.0f)
        , width(0)
        , height(0)
        , lastRequestedFrame(0)
        , requested(false)
    {
    }

    ~Job()
    {
        if (updater)
            updater->unbind();
    }
};

ViewportRenderer::ViewportRenderer(VisualEngine* visualEngine)
    : visualEngine(visualEngine)
    , frameNumber(0)
{
}

ViewportRenderer::~ViewportRenderer()
{
    clear();
}

TextureRef ViewportRenderer::request(const ViewportTextureRequest& request)
{
    if (!request.world)
        return TextureRef();

    const JobKey key(request.world.get(), request.cacheKey);
    boost::shared_ptr<Job>& slot = jobs[key];
    if (!slot)
    {
        DataModel* dataModel = DataModel::get(request.world.get());
        if (!dataModel)
            return TextureRef();

        slot.reset(new Job());
        slot->world = request.world;
        slot->updater.reset(new SceneUpdater(shared_from(dataModel), visualEngine,
            request.world, visualEngine->getSceneManager(), request.world.get()));
    }

    Job& job = *slot;
    const unsigned width = std::max(1u, std::min(2048u,
        static_cast<unsigned>(std::ceil(std::max(1.0f, request.size.x)))));
    const unsigned height = std::max(1u, std::min(2048u,
        static_cast<unsigned>(std::ceil(std::max(1.0f, request.size.y)))));

    if (!job.texture || job.width != width || job.height != height)
    {
        job.texture = visualEngine->getDevice()->createTexture(Texture::Type_2D,
            Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
        shared_ptr<Renderbuffer> depth = visualEngine->getDevice()->createRenderbuffer(
            Texture::Format_D24S8, width, height, 1);
        job.framebuffer = visualEngine->getDevice()->createFramebuffer(
            job.texture->getRenderbuffer(0, 0), depth);
        job.textureRef = TextureRef(job.texture);
        job.width = width;
        job.height = height;
    }

    job.cameraCFrame = request.camera ? request.camera->getCameraCoordinateFrame() : request.cameraCFrame;
    job.fieldOfView = request.camera ? request.camera->getFieldOfViewDegrees() : request.fieldOfView;
    job.ambient = request.ambient;
    job.lightColor = request.lightColor;
    job.lightDirection = request.lightDirection;
    job.lastRequestedFrame = frameNumber;
    job.requested = true;
    return job.textureRef.clone();
}

void ViewportRenderer::render(DeviceContext* context)
{
    ++frameNumber;
    for (std::map<JobKey, boost::shared_ptr<Job> >::iterator it = jobs.begin();
        it != jobs.end(); )
    {
        boost::shared_ptr<Job> job = it->second;
        if (!job->world || !DataModel::get(job->world.get()) ||
            (!job->requested && frameNumber - job->lastRequestedFrame > 120))
        {
            it = jobs.erase(it);
            continue;
        }

        if (job->requested && job->framebuffer)
        {
            RenderCamera camera;
            camera.setViewCFrame(job->cameraCFrame);
            camera.setProjectionPerspective(G3D::toRadians(job->fieldOfView),
                static_cast<float>(job->width) / static_cast<float>(job->height), 0.1f, 5000.0f);

            job->updater->updatePrepare(frameNumber, *visualEngine->getUpdateFrustum());
            job->updater->updatePerform();
            visualEngine->getSceneManager()->renderViewport(context, job->framebuffer.get(),
                camera, job->width, job->height, job->world.get(), Color4::clear(),
                job->ambient, job->lightColor, job->lightDirection);
        }

        job->requested = false;
        ++it;
    }
}

void ViewportRenderer::clear()
{
    jobs.clear();
}

} // namespace Graphics
} // namespace RBX
