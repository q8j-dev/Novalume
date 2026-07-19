#pragma once

#include "V8Tree/Service.h"

#include "rbx/signal.h"

#include <map>
#include <string>

namespace RBX {

extern const char* const sVideoCaptureService;

class VideoCaptureService
    : public DescribedNonCreatable<VideoCaptureService, Instance, sVideoCaptureService>
    , public Service
{
public:
    VideoCaptureService();

    bool getActive() const { return active; }
    void setActive(bool value);
    std::string getCameraID() const { return cameraId; }
    void setCameraID(std::string value);
    shared_ptr<const Reflection::ValueTable> getCameraDevices();

    // Platform capture hosts publish actual device and lifecycle changes here.
    void setCameraDevices(const std::map<std::string, std::string>& devices);
    void reportStarted(const std::string& cameraId);
    void reportStopped(const std::string& cameraId);
    void reportError(const std::string& cameraId, const std::string& errorCode);

    rbx::signal<void()> devicesChangedSignal;
    rbx::signal<void(std::string, std::string)> errorSignal;
    rbx::signal<void(std::string)> startedSignal;
    rbx::signal<void(std::string)> stoppedSignal;
    rbx::signal<void(std::string)> startCaptureRequested;
    rbx::signal<void(std::string)> stopCaptureRequested;

private:
    bool active;
    std::string cameraId;
    std::map<std::string, std::string> cameraDevices;
};

} // namespace RBX
