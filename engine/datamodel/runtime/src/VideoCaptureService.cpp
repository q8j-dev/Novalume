#include "v8datamodel/VideoCaptureService.h"

namespace RBX {

const char* const sVideoCaptureService = "VideoCaptureService";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<VideoCaptureService, bool> propActive(
    "Active", category_Data, &VideoCaptureService::getActive,
    &VideoCaptureService::setActive, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING,
    Security::RobloxScript);
static Reflection::PropDescriptor<VideoCaptureService, std::string> propCameraID(
    "CameraID", category_Data, &VideoCaptureService::getCameraID,
    &VideoCaptureService::setCameraID, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING,
    Security::RobloxScript);
static Reflection::BoundFuncDesc<VideoCaptureService,
    shared_ptr<const Reflection::ValueTable>()> funcGetCameraDevices(
        &VideoCaptureService::getCameraDevices, "GetCameraDevices", Security::RobloxScript);
static Reflection::EventDesc<VideoCaptureService, void()> eventDevicesChanged(
    &VideoCaptureService::devicesChangedSignal, "DevicesChanged", Security::RobloxScript);
static Reflection::EventDesc<VideoCaptureService, void(std::string, std::string)> eventError(
    &VideoCaptureService::errorSignal, "Error", "cameraid", "errorcode",
    Security::RobloxScript);
static Reflection::EventDesc<VideoCaptureService, void(std::string)> eventStarted(
    &VideoCaptureService::startedSignal, "Started", "cameraid", Security::RobloxScript);
static Reflection::EventDesc<VideoCaptureService, void(std::string)> eventStopped(
    &VideoCaptureService::stoppedSignal, "Stopped", "cameraid", Security::RobloxScript);
REFLECTION_END();

VideoCaptureService::VideoCaptureService()
    : Service(true)
    , active(false)
{
    setName(sVideoCaptureService);
    setRobloxLocked(true);
}

void VideoCaptureService::setActive(bool value)
{
    if (active == value)
        return;
    active = value;
    raisePropertyChanged(propActive);
    if (value)
        startCaptureRequested(cameraId);
    else
        stopCaptureRequested(cameraId);
}

void VideoCaptureService::setCameraID(std::string value)
{
    if (cameraId == value)
        return;
    const std::string oldId = cameraId;
    cameraId = value;
    raisePropertyChanged(propCameraID);
    if (active)
    {
        stopCaptureRequested(oldId);
        startCaptureRequested(cameraId);
    }
}

shared_ptr<const Reflection::ValueTable> VideoCaptureService::getCameraDevices()
{
    shared_ptr<Reflection::ValueTable> result(new Reflection::ValueTable());
    for (std::map<std::string, std::string>::const_iterator it = cameraDevices.begin();
         it != cameraDevices.end(); ++it)
        (*result)[it->first] = Reflection::Variant(it->second);
    return result;
}

void VideoCaptureService::setCameraDevices(
    const std::map<std::string, std::string>& devices)
{
    if (cameraDevices == devices)
        return;
    cameraDevices = devices;
    if (!cameraId.empty() && cameraDevices.find(cameraId) == cameraDevices.end())
    {
        const std::string removedId = cameraId;
        cameraId.clear();
        raisePropertyChanged(propCameraID);
        if (active)
            reportStopped(removedId);
    }
    devicesChangedSignal();
}

void VideoCaptureService::reportStarted(const std::string& id)
{
    if (cameraId != id)
    {
        cameraId = id;
        raisePropertyChanged(propCameraID);
    }
    if (!active)
    {
        active = true;
        raisePropertyChanged(propActive);
    }
    startedSignal(id);
}

void VideoCaptureService::reportStopped(const std::string& id)
{
    if (active)
    {
        active = false;
        raisePropertyChanged(propActive);
    }
    stoppedSignal(id);
}

void VideoCaptureService::reportError(const std::string& id,
    const std::string& errorCode)
{
    errorSignal(id, errorCode);
}

} // namespace RBX
