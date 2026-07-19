#include "V8DataModel/VRService.h"

#include "V8DataModel/DataModel.h"

namespace RBX {

const char* const sVRService = "VRService";

namespace Reflection {
template<> EnumDesc<VRService::VRScaling>::EnumDesc() : EnumDescriptor("VRScaling")
{
    addPair(VRService::VRSCALING_OFF, "Off");
    addPair(VRService::VRSCALING_WORLD, "World");
}
template<> EnumDesc<VRService::VRControllerModelMode>::EnumDesc() : EnumDescriptor("VRControllerModelMode")
{
    addPair(VRService::VRCONTROLLER_TRANSPARENT, "Transparent");
    addPair(VRService::VRCONTROLLER_HIDDEN, "Hidden");
}
template<> EnumDesc<VRService::VRLaserPointerMode>::EnumDesc() : EnumDescriptor("VRLaserPointerMode")
{
    addPair(VRService::VRLASER_POINTER_DISABLED, "Disabled");
    addPair(VRService::VRLASER_POINTER_POINTER, "Pointer");
    addPair(VRService::VRLASER_POINTER_NAVIGATION, "Navigation");
}
template<> EnumDesc<VRService::VRTouchpad>::EnumDesc() : EnumDescriptor("VRTouchpad")
{
    addPair(VRService::VRTOUCHPAD_LEFT, "Left");
    addPair(VRService::VRTOUCHPAD_RIGHT, "Right");
}
template<> EnumDesc<VRService::VRTouchpadMode>::EnumDesc() : EnumDescriptor("VRTouchpadMode")
{
    addPair(VRService::VRTOUCHPAD_MODE_TOUCH, "Touch");
    addPair(VRService::VRTOUCHPAD_MODE_VIRTUAL_THUMBSTICK, "VirtualThumbstick");
    addPair(VRService::VRTOUCHPAD_MODE_ABXY, "ABXY");
}

template<> VRService::VRScaling& Variant::convert<VRService::VRScaling>() { return genericConvert<VRService::VRScaling>(); }
template<> VRService::VRControllerModelMode& Variant::convert<VRService::VRControllerModelMode>() { return genericConvert<VRService::VRControllerModelMode>(); }
template<> VRService::VRLaserPointerMode& Variant::convert<VRService::VRLaserPointerMode>() { return genericConvert<VRService::VRLaserPointerMode>(); }
template<> VRService::VRTouchpad& Variant::convert<VRService::VRTouchpad>() { return genericConvert<VRService::VRTouchpad>(); }
template<> VRService::VRTouchpadMode& Variant::convert<VRService::VRTouchpadMode>() { return genericConvert<VRService::VRTouchpadMode>(); }
} // namespace Reflection

template<> bool StringConverter<VRService::VRScaling>::convertToValue(const std::string& text, VRService::VRScaling& value) { return Reflection::EnumDesc<VRService::VRScaling>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<VRService::VRControllerModelMode>::convertToValue(const std::string& text, VRService::VRControllerModelMode& value) { return Reflection::EnumDesc<VRService::VRControllerModelMode>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<VRService::VRLaserPointerMode>::convertToValue(const std::string& text, VRService::VRLaserPointerMode& value) { return Reflection::EnumDesc<VRService::VRLaserPointerMode>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<VRService::VRTouchpad>::convertToValue(const std::string& text, VRService::VRTouchpad& value) { return Reflection::EnumDesc<VRService::VRTouchpad>::singleton().convertToValue(text.c_str(), value); }
template<> bool StringConverter<VRService::VRTouchpadMode>::convertToValue(const std::string& text, VRService::VRTouchpadMode& value) { return Reflection::EnumDesc<VRService::VRTouchpadMode>::singleton().convertToValue(text.c_str(), value); }

REFLECTION_BEGIN();
static Reflection::PropDescriptor<VRService, bool> propVREnabled("VREnabled", category_Data, &VRService::getVREnabled, NULL);
static Reflection::PropDescriptor<VRService, bool> propVRDeviceAvailable("VRDeviceAvailable", category_Data, &VRService::getVRDeviceAvailable, NULL, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::EnumPropDescriptor<VRService, UserInputService::UserCFrame> propGuiInput("GuiInputUserCFrame", category_Data, &VRService::getGuiInputUserCFrame, &VRService::setGuiInputUserCFrame);
static Reflection::PropDescriptor<VRService, bool> propAvatarGestures("AvatarGestures", category_Behavior, &VRService::getAvatarGestures, &VRService::setAvatarGestures);
static Reflection::PropDescriptor<VRService, bool> propThirdPerson("ThirdPersonFollowCamEnabled", category_Behavior, &VRService::getThirdPersonFollowCamEnabled, &VRService::setThirdPersonFollowCamEnabled);
static Reflection::PropDescriptor<VRService, bool> propFade("FadeOutViewOnCollision", category_Behavior, &VRService::getFadeOutViewOnCollision, &VRService::setFadeOutViewOnCollision);
static Reflection::EnumPropDescriptor<VRService, VRService::VRScaling> propScaling("AutomaticScaling", category_Behavior, &VRService::getAutomaticScaling, &VRService::setAutomaticScaling);
static Reflection::EnumPropDescriptor<VRService, VRService::VRControllerModelMode> propControllers("ControllerModels", category_Behavior, &VRService::getControllerModels, &VRService::setControllerModels);
static Reflection::EnumPropDescriptor<VRService, VRService::VRLaserPointerMode> propLaser("LaserPointer", category_Behavior, &VRService::getLaserPointer, &VRService::setLaserPointer);
static Reflection::BoundFuncDesc<VRService, CoordinateFrame(UserInputService::UserCFrame)> funcGetUserCFrame(&VRService::getUserCFrame, "GetUserCFrame", "type", Security::None);
static Reflection::BoundFuncDesc<VRService, bool(UserInputService::UserCFrame)> funcGetUserCFrameEnabled(&VRService::getUserCFrameEnabled, "GetUserCFrameEnabled", "type", Security::None);
static Reflection::BoundFuncDesc<VRService, void()> funcRecenter(&VRService::recenterUserHeadCFrame, "RecenterUserHeadCFrame", Security::None);
static Reflection::BoundFuncDesc<VRService, bool()> funcIsMaquettes(&VRService::isMaquettes, "IsMaquettes", Security::RobloxScript);
static Reflection::BoundFuncDesc<VRService, bool()> funcIsVRAppBuild(&VRService::isVRAppBuild, "IsVRAppBuild", Security::RobloxScript);
static Reflection::BoundFuncDesc<VRService, VRService::VRTouchpadMode(VRService::VRTouchpad)> funcGetTouchpad(&VRService::getTouchpadMode, "GetTouchpadMode", "pad", Security::None);
static Reflection::BoundFuncDesc<VRService, void(VRService::VRTouchpad, VRService::VRTouchpadMode)> funcSetTouchpad(&VRService::setTouchpadMode, "SetTouchpadMode", "pad", "mode", Security::None);
static Reflection::BoundFuncDesc<VRService, void(CoordinateFrame, UserInputService::UserCFrame)> funcRequestNavigation(&VRService::requestNavigation, "RequestNavigation", "cframe", "inputUserCFrame", Security::None);
static Reflection::EventDesc<VRService, void(UserInputService::UserCFrame, CoordinateFrame)> eventUserCFrameChanged(&VRService::userCFrameChanged, "UserCFrameChanged", "type", "value", Security::None);
static Reflection::EventDesc<VRService, void(UserInputService::UserCFrame, bool)> eventUserCFrameEnabled(&VRService::userCFrameEnabled, "UserCFrameEnabled", "type", "enabled", Security::None);
static Reflection::EventDesc<VRService, void(CoordinateFrame, UserInputService::UserCFrame)> eventNavigation(&VRService::navigationRequested, "NavigationRequested", "cframe", "inputUserCFrame", Security::None);
static Reflection::EventDesc<VRService, void(VRService::VRTouchpad, VRService::VRTouchpadMode)> eventTouchpad(&VRService::touchpadModeChanged, "TouchpadModeChanged", "pad", "mode", Security::None);
static Reflection::EventDesc<VRService, void(shared_ptr<Instance>)> eventLaserPointer(&VRService::laserPointerTriggered, "LaserPointerTriggered", "input", Security::None);
REFLECTION_END();

VRService::VRService()
    : Service(true)
    , guiInputUserCFrame(UserInputService::USERCFRAME_HEAD)
    , avatarGestures(false)
    , thirdPersonFollowCamEnabled(false)
    , fadeOutViewOnCollision(true)
    , automaticScaling(VRSCALING_WORLD)
    , controllerModels(VRCONTROLLER_TRANSPARENT)
    , laserPointer(VRLASER_POINTER_POINTER)
    , touchpadModes{VRTOUCHPAD_MODE_TOUCH, VRTOUCHPAD_MODE_TOUCH}
{
    setName(sVRService);
}

UserInputService* VRService::inputService() const
{
    return ServiceProvider::find<UserInputService>(this);
}

bool VRService::getVREnabled() const { const UserInputService* service = inputService(); return service && service->getVREnabled(); }
bool VRService::getVRDeviceAvailable() const { const UserInputService* service = inputService(); return service && service->getVREnabled(); }
CoordinateFrame VRService::getUserCFrame(UserInputService::UserCFrame type) { const UserInputService* service = inputService(); return service ? service->getUserCFrame(type) : CoordinateFrame(); }
bool VRService::getUserCFrameEnabled(UserInputService::UserCFrame) { return getVREnabled(); }
void VRService::recenterUserHeadCFrame() { if (UserInputService* service = inputService()) service->recenterUserHeadCFrame(); }
VRService::VRTouchpadMode VRService::getTouchpadMode(VRTouchpad pad) { return touchpadModes.at(static_cast<std::size_t>(pad)); }
void VRService::setTouchpadMode(VRTouchpad pad, VRTouchpadMode mode) { touchpadModes.at(static_cast<std::size_t>(pad)) = mode; touchpadModeChanged(pad, mode); }
void VRService::requestNavigation(CoordinateFrame cframe, UserInputService::UserCFrame input) { navigationRequested(cframe, input); }

#define RBX_VR_SETTER(name, field, type) void VRService::name(type value) { if (field != value) { field = value; raiseChanged(prop##field); } }
void VRService::setGuiInputUserCFrame(UserInputService::UserCFrame value) { guiInputUserCFrame = value; }
void VRService::setAvatarGestures(bool value) { avatarGestures = value; }
void VRService::setThirdPersonFollowCamEnabled(bool value) { thirdPersonFollowCamEnabled = value; }
void VRService::setFadeOutViewOnCollision(bool value) { fadeOutViewOnCollision = value; }
void VRService::setAutomaticScaling(VRScaling value) { automaticScaling = value; }
void VRService::setControllerModels(VRControllerModelMode value) { controllerModels = value; }
void VRService::setLaserPointer(VRLaserPointerMode value) { laserPointer = value; }
#undef RBX_VR_SETTER

} // namespace RBX
