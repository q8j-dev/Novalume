#pragma once

#include "V8DataModel/UserInputService.h"
#include "V8Tree/Service.h"

#include <array>

namespace RBX {

extern const char* const sVRService;

class VRService
    : public DescribedNonCreatable<VRService, Instance, sVRService>
    , public Service
{
public:
    enum VRScaling { VRSCALING_OFF, VRSCALING_WORLD };
    enum VRControllerModelMode { VRCONTROLLER_TRANSPARENT, VRCONTROLLER_HIDDEN };
    enum VRLaserPointerMode { VRLASER_POINTER_DISABLED, VRLASER_POINTER_POINTER, VRLASER_POINTER_NAVIGATION };
    enum VRTouchpad { VRTOUCHPAD_LEFT, VRTOUCHPAD_RIGHT };
    enum VRTouchpadMode { VRTOUCHPAD_MODE_TOUCH, VRTOUCHPAD_MODE_VIRTUAL_THUMBSTICK, VRTOUCHPAD_MODE_ABXY };

    VRService();

    bool getVREnabled() const;
    bool getVRDeviceAvailable() const;
    UserInputService::UserCFrame getGuiInputUserCFrame() const { return guiInputUserCFrame; }
    void setGuiInputUserCFrame(UserInputService::UserCFrame value);
    bool getAvatarGestures() const { return avatarGestures; }
    void setAvatarGestures(bool value);
    bool getThirdPersonFollowCamEnabled() const { return thirdPersonFollowCamEnabled; }
    void setThirdPersonFollowCamEnabled(bool value);
    bool getFadeOutViewOnCollision() const { return fadeOutViewOnCollision; }
    void setFadeOutViewOnCollision(bool value);
    VRScaling getAutomaticScaling() const { return automaticScaling; }
    void setAutomaticScaling(VRScaling value);
    VRControllerModelMode getControllerModels() const { return controllerModels; }
    void setControllerModels(VRControllerModelMode value);
    VRLaserPointerMode getLaserPointer() const { return laserPointer; }
    void setLaserPointer(VRLaserPointerMode value);

    CoordinateFrame getUserCFrame(UserInputService::UserCFrame type);
    bool getUserCFrameEnabled(UserInputService::UserCFrame type);
    void recenterUserHeadCFrame();
    bool isMaquettes() { return false; }
    bool isVRAppBuild() { return false; }
    VRTouchpadMode getTouchpadMode(VRTouchpad pad);
    void setTouchpadMode(VRTouchpad pad, VRTouchpadMode mode);
    void requestNavigation(CoordinateFrame cframe, UserInputService::UserCFrame input);

    rbx::signal<void(UserInputService::UserCFrame, CoordinateFrame)> userCFrameChanged;
    rbx::signal<void(UserInputService::UserCFrame, bool)> userCFrameEnabled;
    rbx::signal<void(CoordinateFrame, UserInputService::UserCFrame)> navigationRequested;
    rbx::signal<void(VRTouchpad, VRTouchpadMode)> touchpadModeChanged;
    rbx::signal<void(shared_ptr<Instance>)> laserPointerTriggered;

private:
    UserInputService* inputService() const;

    UserInputService::UserCFrame guiInputUserCFrame;
    bool avatarGestures;
    bool thirdPersonFollowCamEnabled;
    bool fadeOutViewOnCollision;
    VRScaling automaticScaling;
    VRControllerModelMode controllerModels;
    VRLaserPointerMode laserPointer;
    std::array<VRTouchpadMode, 2> touchpadModes;
    rbx::signals::scoped_connection userCFrameConnection;
};

} // namespace RBX
