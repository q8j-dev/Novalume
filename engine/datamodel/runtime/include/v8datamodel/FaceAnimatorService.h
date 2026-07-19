#pragma once

#include "V8DataModel/InteractionEnums.h"
#include "V8Tree/Service.h"

#include "rbx/signal.h"

namespace RBX {

extern const char* const sFaceAnimatorService;
extern const char* const sTrackerLodController;

class TrackerLodController
    : public DescribedNonCreatable<TrackerLodController, Instance, sTrackerLodController>
{
public:
    TrackerLodController();
    Enums::TrackerLodFlagMode getAudioMode() const { return audioMode; }
    void setAudioMode(Enums::TrackerLodFlagMode value);
    Enums::TrackerExtrapolationFlagMode getVideoExtrapolationMode() const { return extrapolationMode; }
    void setVideoExtrapolationMode(Enums::TrackerExtrapolationFlagMode value);
    Enums::TrackerLodValueMode getVideoLodMode() const { return videoLodMode; }
    void setVideoLodMode(Enums::TrackerLodValueMode value);
    Enums::TrackerLodFlagMode getVideoMode() const { return videoMode; }
    void setVideoMode(Enums::TrackerLodFlagMode value);
    int getExtrapolation();
    int getVideoLod();
    bool isAudioEnabled();
    bool isVideoEnabled();
    void setSourceStates(bool audio, bool video);
    rbx::signal<void()> updateStateSignal;
private:
    Enums::TrackerLodFlagMode audioMode;
    Enums::TrackerExtrapolationFlagMode extrapolationMode;
    Enums::TrackerLodValueMode videoLodMode;
    Enums::TrackerLodFlagMode videoMode;
    bool sourceAudio;
    bool sourceVideo;
};

class FaceAnimatorService
    : public DescribedNonCreatable<FaceAnimatorService, Instance, sFaceAnimatorService>
    , public Service
{
public:
    FaceAnimatorService();
    bool getAudioAnimationEnabled() const { return audioEnabled; }
    void setAudioAnimationEnabled(bool value);
    Enums::TrackerFaceTrackingStatus getFaceTrackingStatusEnum() const { return faceStatus; }
    void setFaceTrackingStatusEnum(Enums::TrackerFaceTrackingStatus value);
    bool getFlipHeadOrientation() const { return flipHeadOrientation; }
    void setFlipHeadOrientation(bool value);
    bool getVideoAnimationEnabled() const { return videoEnabled; }
    void setVideoAnimationEnabled(bool value);
    shared_ptr<Instance> getTrackerLodController();
    void init(bool videoEnabled, bool audioEnabled);
    bool isStarted();
    void start();
    void step();
    void stop();
    rbx::signal<void(Enums::TrackerError)> trackerErrorSignal;
    rbx::signal<void(Enums::TrackerPromptEvent)> trackerPromptSignal;
    rbx::signal<void(bool, bool)> initializeTrackerRequested;
    rbx::signal<void()> startTrackerRequested;
    rbx::signal<void()> stepTrackerRequested;
    rbx::signal<void()> stopTrackerRequested;
private:
    void updateController();
    bool audioEnabled;
    bool videoEnabled;
    bool flipHeadOrientation;
    bool initialized;
    bool started;
    Enums::TrackerFaceTrackingStatus faceStatus;
    shared_ptr<TrackerLodController> lodController;
};

} // namespace RBX
