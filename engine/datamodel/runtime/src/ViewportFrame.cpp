#include "v8datamodel/ViewportFrame.h"

#include "v8datamodel/Camera.h"
#include "v8datamodel/WorldModel.h"
#include "GfxBase/ViewportTextureProvider.h"

namespace RBX {

const char* const sViewportFrame = "ViewportFrame";

static Reflection::PropDescriptor<ViewportFrame, Color3> prop_ViewportAmbient(
    "Ambient", category_Appearance, &ViewportFrame::getAmbient, &ViewportFrame::setAmbient);
static Reflection::PropDescriptor<ViewportFrame, Color3> prop_ViewportImageColor(
    "ImageColor3", category_Appearance, &ViewportFrame::getImageColor3, &ViewportFrame::setImageColor3);
static Reflection::PropDescriptor<ViewportFrame, float> prop_ViewportImageTransparency(
    "ImageTransparency", category_Appearance, &ViewportFrame::getImageTransparency, &ViewportFrame::setImageTransparency);
static Reflection::PropDescriptor<ViewportFrame, Color3> prop_ViewportLightColor(
    "LightColor", category_Appearance, &ViewportFrame::getLightColor, &ViewportFrame::setLightColor);
static Reflection::PropDescriptor<ViewportFrame, Vector3> prop_ViewportLightDirection(
    "LightDirection", category_Appearance, &ViewportFrame::getLightDirection, &ViewportFrame::setLightDirection);
static Reflection::RefPropDescriptor<ViewportFrame, Camera> prop_ViewportCurrentCamera(
    "CurrentCamera", category_Data, &ViewportFrame::getCurrentCamera, &ViewportFrame::setCurrentCamera,
    Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
static Reflection::PropDescriptor<ViewportFrame, CoordinateFrame> prop_ViewportCameraCFrame(
    "CameraCFrame", category_Data, &ViewportFrame::getCameraCFrame, &ViewportFrame::setCameraCFrame);
static Reflection::PropDescriptor<ViewportFrame, float> prop_ViewportCameraFieldOfView(
    "CameraFieldOfView", category_Data, &ViewportFrame::getCameraFieldOfView, &ViewportFrame::setCameraFieldOfView);
static Reflection::PropDescriptor<ViewportFrame, bool> prop_ViewportIsMirrored(
    "IsMirrored", category_Data, &ViewportFrame::getIsMirrored, &ViewportFrame::setIsMirrored);

ViewportFrame::ViewportFrame()
    : DescribedCreatable<ViewportFrame, GuiObject, sViewportFrame>(sViewportFrame, false)
    , ambient(0.784f, 0.784f, 0.784f)
    , imageColor(Color3::white())
    , imageTransparency(0.0f)
    , lightColor(Color3::white())
    , lightDirection(-1.0f, -1.0f, -1.0f)
    , cameraFieldOfView(70.0f)
    , mirrored(false)
{
}

void ViewportFrame::setAmbient(Color3 value)
{
    if (ambient != value) { ambient = value; raisePropertyChanged(prop_ViewportAmbient); }
}

void ViewportFrame::setImageColor3(Color3 value)
{
    if (imageColor != value) { imageColor = value; raisePropertyChanged(prop_ViewportImageColor); }
}

void ViewportFrame::setImageTransparency(float value)
{
    value = G3D::clamp(value, 0.0f, 1.0f);
    if (imageTransparency != value) { imageTransparency = value; raisePropertyChanged(prop_ViewportImageTransparency); }
}

void ViewportFrame::setLightColor(Color3 value)
{
    if (lightColor != value) { lightColor = value; raisePropertyChanged(prop_ViewportLightColor); }
}

void ViewportFrame::setLightDirection(Vector3 value)
{
    if (lightDirection != value) { lightDirection = value; raisePropertyChanged(prop_ViewportLightDirection); }
}

void ViewportFrame::setCurrentCamera(Camera* value)
{
    if (currentCamera.get() != value)
    {
        currentCamera = shared_from(value);
        raisePropertyChanged(prop_ViewportCurrentCamera);
    }
}

void ViewportFrame::setCameraCFrame(const CoordinateFrame& value)
{
    if (cameraCFrame != value) { cameraCFrame = value; raisePropertyChanged(prop_ViewportCameraCFrame); }
}

void ViewportFrame::setCameraFieldOfView(float value)
{
    value = G3D::clamp(value, 1.0f, 120.0f);
    if (cameraFieldOfView != value) { cameraFieldOfView = value; raisePropertyChanged(prop_ViewportCameraFieldOfView); }
}

void ViewportFrame::setIsMirrored(bool value)
{
    if (mirrored != value) { mirrored = value; raisePropertyChanged(prop_ViewportIsMirrored); }
}

void ViewportFrame::render2d(Adorn* adorn)
{
    ViewportTextureProvider* provider = dynamic_cast<ViewportTextureProvider*>(adorn);
    WorldModel* world = findFirstChildOfType<WorldModel>();
    if (!provider || !world || imageTransparency >= 1.0f)
        return;

    ViewportTextureRequest request;
    request.world = shared_from<Instance>(world);
    request.camera = currentCamera.get();
    request.cameraCFrame = cameraCFrame;
    request.fieldOfView = cameraFieldOfView;
    request.size = getAbsoluteSize();
    request.ambient = ambient;
    request.lightColor = lightColor;
    request.lightDirection = lightDirection;
    request.mirrored = mirrored;

    TextureProxyBaseRef texture = provider->requestViewportTexture(request);
    if (!texture)
        return;

    adorn->setTexture(0, texture);
    const float top = provider->requiresViewportTextureFlipping() ? 1.0f : 0.0f;
    const float bottom = 1.0f - top;
    const Vector2 uv0(mirrored ? 1.0f : 0.0f, top);
    const Vector2 uv1(mirrored ? 0.0f : 1.0f, bottom);
    const Color4 color = applyCanvasGroup(Color4(imageColor, 1.0f - imageTransparency));
    GuiObject* clippingObject = firstAncestorClipping();
    if (clippingObject && getAbsoluteRotation().empty())
        adorn->rect2d(getRect2D(), uv0, uv1, color, clippingObject->getClippedRect());
    else
        adorn->rect2d(getRect2D(), uv0, uv1, color, getAbsoluteRotation());
    adorn->setTexture(0, TextureProxyBaseRef());
}

void ViewportFrame::renderBackground2d(Adorn* adorn)
{
    if (getBackgroundTransparency() < 1.0f)
        render2dImpl(adorn, getRenderBackgroundColor4());
}

} // namespace RBX
