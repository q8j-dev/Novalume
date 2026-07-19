#include "V8DataModel/FaceAnimatorService.h"

namespace RBX {
const char* const sFaceAnimatorService = "FaceAnimatorService";
const char* const sTrackerLodController = "TrackerLodController";

REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<TrackerLodController, Enums::TrackerLodFlagMode> propAudioMode("AudioMode", category_Control, &TrackerLodController::getAudioMode, &TrackerLodController::setAudioMode);
static Reflection::EnumPropDescriptor<TrackerLodController, Enums::TrackerExtrapolationFlagMode> propVideoExtrapolationMode("VideoExtrapolationMode", category_Control, &TrackerLodController::getVideoExtrapolationMode, &TrackerLodController::setVideoExtrapolationMode);
static Reflection::EnumPropDescriptor<TrackerLodController, Enums::TrackerLodValueMode> propVideoLodMode("VideoLodMode", category_Control, &TrackerLodController::getVideoLodMode, &TrackerLodController::setVideoLodMode);
static Reflection::EnumPropDescriptor<TrackerLodController, Enums::TrackerLodFlagMode> propVideoMode("VideoMode", category_Control, &TrackerLodController::getVideoMode, &TrackerLodController::setVideoMode);
static Reflection::BoundFuncDesc<TrackerLodController, int()> funcGetExtrapolation(&TrackerLodController::getExtrapolation, "getExtrapolation", Security::RobloxScript);
static Reflection::BoundFuncDesc<TrackerLodController, int()> funcGetVideoLod(&TrackerLodController::getVideoLod, "getVideoLod", Security::RobloxScript);
static Reflection::BoundFuncDesc<TrackerLodController, bool()> funcIsAudioEnabled(&TrackerLodController::isAudioEnabled, "isAudioEnabled", Security::RobloxScript);
static Reflection::BoundFuncDesc<TrackerLodController, bool()> funcIsVideoEnabled(&TrackerLodController::isVideoEnabled, "isVideoEnabled", Security::RobloxScript);
static Reflection::EventDesc<TrackerLodController, void()> eventUpdateState(&TrackerLodController::updateStateSignal, "UpdateState", Security::RobloxScript);

static Reflection::PropDescriptor<FaceAnimatorService, bool> propAudioAnimationEnabled("AudioAnimationEnabled", category_Control, &FaceAnimatorService::getAudioAnimationEnabled, &FaceAnimatorService::setAudioAnimationEnabled, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::EnumPropDescriptor<FaceAnimatorService, Enums::TrackerFaceTrackingStatus> propFaceTrackingStatusEnum("FaceTrackingStatusEnum", category_Control, &FaceAnimatorService::getFaceTrackingStatusEnum, &FaceAnimatorService::setFaceTrackingStatusEnum, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<FaceAnimatorService, bool> propFlipHeadOrientation("FlipHeadOrientation", category_Control, &FaceAnimatorService::getFlipHeadOrientation, &FaceAnimatorService::setFlipHeadOrientation, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::PropDescriptor<FaceAnimatorService, bool> propVideoAnimationEnabled("VideoAnimationEnabled", category_Control, &FaceAnimatorService::getVideoAnimationEnabled, &FaceAnimatorService::setVideoAnimationEnabled, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::RobloxScript);
static Reflection::BoundFuncDesc<FaceAnimatorService, shared_ptr<Instance>()> funcGetTrackerLodController(&FaceAnimatorService::getTrackerLodController, "GetTrackerLodController", Security::RobloxScript);
static Reflection::BoundFuncDesc<FaceAnimatorService, void(bool, bool)> funcInit(&FaceAnimatorService::init, "Init", "videoEnabled", "audioEnabled", Security::RobloxScript);
static Reflection::BoundFuncDesc<FaceAnimatorService, bool()> funcIsStarted(&FaceAnimatorService::isStarted, "IsStarted", Security::RobloxScript);
static Reflection::BoundFuncDesc<FaceAnimatorService, void()> funcStart(&FaceAnimatorService::start, "Start", Security::RobloxScript);
static Reflection::BoundFuncDesc<FaceAnimatorService, void()> funcStep(&FaceAnimatorService::step, "Step", Security::RobloxScript);
static Reflection::BoundFuncDesc<FaceAnimatorService, void()> funcStop(&FaceAnimatorService::stop, "Stop", Security::RobloxScript);
static Reflection::EventDesc<FaceAnimatorService, void(Enums::TrackerError)> eventTrackerError(&FaceAnimatorService::trackerErrorSignal, "TrackerError", "error", Security::RobloxScript);
static Reflection::EventDesc<FaceAnimatorService, void(Enums::TrackerPromptEvent)> eventTrackerPrompt(&FaceAnimatorService::trackerPromptSignal, "TrackerPrompt", "prompt", Security::RobloxScript);
REFLECTION_END();

TrackerLodController::TrackerLodController() : audioMode(Enums::TRACKER_LOD_FLAG_AUTO), extrapolationMode(Enums::TRACKER_EXTRAPOLATION_AUTO), videoLodMode(Enums::TRACKER_LOD_VALUE_AUTO), videoMode(Enums::TRACKER_LOD_FLAG_AUTO), sourceAudio(false), sourceVideo(false) { setName(sTrackerLodController); setRobloxLocked(true); }
#define RBX_LOD_SETTER(Name, Type, field, descriptor) void TrackerLodController::set##Name(Type value) { if (field != value) { field = value; raisePropertyChanged(descriptor); updateStateSignal(); } }
RBX_LOD_SETTER(AudioMode, Enums::TrackerLodFlagMode, audioMode, propAudioMode)
RBX_LOD_SETTER(VideoExtrapolationMode, Enums::TrackerExtrapolationFlagMode, extrapolationMode, propVideoExtrapolationMode)
RBX_LOD_SETTER(VideoLodMode, Enums::TrackerLodValueMode, videoLodMode, propVideoLodMode)
RBX_LOD_SETTER(VideoMode, Enums::TrackerLodFlagMode, videoMode, propVideoMode)
#undef RBX_LOD_SETTER
int TrackerLodController::getExtrapolation() { return extrapolationMode == Enums::TRACKER_EXTRAPOLATION_FORCE_DISABLED ? 0 : static_cast<int>(extrapolationMode); }
int TrackerLodController::getVideoLod() { return videoLodMode == Enums::TRACKER_LOD_VALUE_FORCE_0 ? 0 : 1; }
bool TrackerLodController::isAudioEnabled() { return audioMode == Enums::TRACKER_LOD_FLAG_FORCE_TRUE || (audioMode == Enums::TRACKER_LOD_FLAG_AUTO && sourceAudio); }
bool TrackerLodController::isVideoEnabled() { return videoMode == Enums::TRACKER_LOD_FLAG_FORCE_TRUE || (videoMode == Enums::TRACKER_LOD_FLAG_AUTO && sourceVideo); }
void TrackerLodController::setSourceStates(bool audio, bool video) { if (sourceAudio != audio || sourceVideo != video) { sourceAudio = audio; sourceVideo = video; updateStateSignal(); } }

FaceAnimatorService::FaceAnimatorService() : Service(true), audioEnabled(false), videoEnabled(false), flipHeadOrientation(false), initialized(false), started(false), faceStatus(Enums::TRACKER_FACE_UNINITIALIZED), lodController(Creatable<Instance>::create<TrackerLodController>()) { setName(sFaceAnimatorService); setRobloxLocked(true); }
void FaceAnimatorService::updateController() { lodController->setSourceStates(started && audioEnabled, started && videoEnabled); }
void FaceAnimatorService::setAudioAnimationEnabled(bool value) { if (audioEnabled != value) { audioEnabled = value; raisePropertyChanged(propAudioAnimationEnabled); updateController(); } }
void FaceAnimatorService::setVideoAnimationEnabled(bool value) { if (videoEnabled != value) { videoEnabled = value; raisePropertyChanged(propVideoAnimationEnabled); updateController(); } }
void FaceAnimatorService::setFlipHeadOrientation(bool value) { if (flipHeadOrientation != value) { flipHeadOrientation = value; raisePropertyChanged(propFlipHeadOrientation); } }
void FaceAnimatorService::setFaceTrackingStatusEnum(Enums::TrackerFaceTrackingStatus value) { if (faceStatus != value) { faceStatus = value; raisePropertyChanged(propFaceTrackingStatusEnum); } }
shared_ptr<Instance> FaceAnimatorService::getTrackerLodController() { return lodController; }
void FaceAnimatorService::init(bool video, bool audio) { initialized = true; setVideoAnimationEnabled(video); setAudioAnimationEnabled(audio); initializeTrackerRequested(video, audio); }
bool FaceAnimatorService::isStarted() { return started; }
void FaceAnimatorService::start() { if (!initialized) init(videoEnabled, audioEnabled); if (!started) { started = true; updateController(); startTrackerRequested(); } }
void FaceAnimatorService::step() { if (started) stepTrackerRequested(); }
void FaceAnimatorService::stop() { if (started) { started = false; faceStatus = Enums::TRACKER_FACE_UNINITIALIZED; raisePropertyChanged(propFaceTrackingStatusEnum); updateController(); stopTrackerRequested(); } }
} // namespace RBX
