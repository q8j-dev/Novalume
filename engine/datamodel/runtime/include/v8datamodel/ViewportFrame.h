#pragma once

#include "v8datamodel/GuiObject.h"

namespace RBX {

class Camera;

extern const char* const sViewportFrame;

class ViewportFrame : public DescribedCreatable<ViewportFrame, GuiObject, sViewportFrame>
{
private:
    typedef DescribedCreatable<ViewportFrame, GuiObject, sViewportFrame> Super;

    Color3 ambient;
    Color3 imageColor;
    float imageTransparency;
    Color3 lightColor;
    Vector3 lightDirection;
    shared_ptr<Camera> currentCamera;
    CoordinateFrame cameraCFrame;
    float cameraFieldOfView;
    bool mirrored;

    /*override*/ void render2d(Adorn* adorn);
    /*override*/ void renderBackground2d(Adorn* adorn);

public:
    ViewportFrame();

    Color3 getAmbient() const { return ambient; }
    void setAmbient(Color3 value);
    Color3 getImageColor3() const { return imageColor; }
    void setImageColor3(Color3 value);
    float getImageTransparency() const { return imageTransparency; }
    void setImageTransparency(float value);
    Color3 getLightColor() const { return lightColor; }
    void setLightColor(Color3 value);
    Vector3 getLightDirection() const { return lightDirection; }
    void setLightDirection(Vector3 value);
    Camera* getCurrentCamera() const { return currentCamera.get(); }
    void setCurrentCamera(Camera* value);
    const CoordinateFrame& getCameraCFrame() const { return cameraCFrame; }
    void setCameraCFrame(const CoordinateFrame& value);
    float getCameraFieldOfView() const { return cameraFieldOfView; }
    void setCameraFieldOfView(float value);
    bool getIsMirrored() const { return mirrored; }
    void setIsMirrored(bool value);
};

} // namespace RBX
