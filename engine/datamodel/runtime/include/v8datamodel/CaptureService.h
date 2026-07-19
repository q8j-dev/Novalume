#pragma once

#include "Util/Content.h"
#include "Util/DateTime.h"
#include "Script/ThreadRef.h"
#include "V8DataModel/InteractionEnums.h"
#include "V8Tree/Service.h"

#include "rbx/signal.h"

#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace RBX {

extern const char* const sCapture;
extern const char* const sCaptureService;

class Capture
    : public Reflection::Described<Capture, sCapture,
          NonFactoryProduct<Reflection::DescribedBase, sCapture>,
          Reflection::ClassDescriptor::RUNTIME_LOCAL>
{
public:
    Capture(std::filesystem::path filePath, DateTime captureTime,
        Enums::CaptureType captureType, std::string localId,
        long long sourcePlaceId, long long sourceUniverseId);

    DateTime getCaptureTime() const { return captureTime; }
    Enums::CaptureType getCaptureType() const { return captureType; }
    std::string getFilePathString() const { return filePath.string(); }
    std::string getLocalId() const { return localId; }
    long long getSourcePlaceId() const { return sourcePlaceId; }
    long long getSourceUniverseId() const { return sourceUniverseId; }
    const std::filesystem::path& getFilePath() const { return filePath; }

private:
    std::filesystem::path filePath;
    DateTime captureTime;
    Enums::CaptureType captureType;
    std::string localId;
    long long sourcePlaceId;
    long long sourceUniverseId;
};

class CaptureService
    : public DescribedNonCreatable<CaptureService, Instance, sCaptureService>
    , public Service
{
public:
    typedef DescribedNonCreatable<CaptureService, Instance, sCaptureService> Super;

    CaptureService();

    bool canCaptureVideo() { return false; }
    bool isCapturingVideo() { return false; }
    void onCaptureBegan();
    void onCaptureEnded();
    void saveScreenshotCapture(std::string additionalInfo);

    shared_ptr<const Reflection::ValueArray> retrieveCaptures();
    shared_ptr<Reflection::DescribedBase> getScreenshotCaptureObject(std::string capturePath);
    void deleteCapture(std::string capturePath);
    std::string preCaptureShared(shared_ptr<Reflection::DescribedBase> capture);
    void onCaptureObjectShared(shared_ptr<Reflection::DescribedBase> capture);
    void onCaptureShared(std::string capturePath);

    int promptSaveCapturesToGallery(lua_State* state);
    int onSavePromptFinished(lua_State* state);
    void promptShareCapture(Content captureContent, std::string launchData,
        Lua::WeakFunctionRef onAcceptedCallback,
        Lua::WeakFunctionRef onDeniedCallback);
    void onSharePromptFinished(long long promptId, bool accepted);

    void getCaptureFilePathAsync(Content captureContent,
        boost::function<void(std::string)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void saveCapturesToExternalStorageAsync(
        shared_ptr<const Reflection::ValueArray> captures,
        boost::function<void(long long)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    void getCaptureSizeAsync(Content captureContent,
        boost::function<void(Vector2)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void getCaptureStorageSizeAsync(shared_ptr<const Reflection::ValueArray> paths,
        boost::function<void(long long)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void deleteCapturesAsync(shared_ptr<const Reflection::ValueArray> paths,
        boost::function<void(long long)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void startVideoCaptureInternalAsync(
        boost::function<void(Enums::VideoCaptureStartedResult)> resumeFunction,
        boost::function<void(std::string)> errorFunction);
    void stopVideoCaptureInternal();

    rbx::signal<void(Enums::CaptureType)> captureBeganSignal;
    rbx::signal<void(Enums::CaptureType)> captureEndedSignal;
    rbx::signal<void(shared_ptr<const Reflection::ValueTable>, std::string)>
        captureSavedInternalSignal;
    rbx::signal<void(shared_ptr<Reflection::DescribedBase>, std::string)>
        captureObjectSavedInternalSignal;
    rbx::signal<void(bool, std::string)> videoCaptureInProgressSignal;
    rbx::signal<void(Enums::VideoCaptureResult)> userVideoCaptureFailedSignal;
    rbx::signal<void(Enums::VideoCaptureStartedResult)>
        userVideoCaptureStartFailedSignal;
    rbx::signal<void(long long, shared_ptr<const Reflection::ValueArray>)>
        openSaveCapturesPromptSignal;
    rbx::signal<void(long long, Content, std::string)>
        openShareCapturePromptSignal;
    rbx::signal<void(Content)> userCaptureSavedSignal;

protected:
    void onServiceProvider(ServiceProvider* oldProvider,
        ServiceProvider* newProvider) override;

private:
    struct SavePrompt
    {
        shared_ptr<const Reflection::ValueArray> captures;
        Lua::WeakFunctionRef callback;
        std::vector<int> originalCaptureRefs;
    };

    struct SharePrompt
    {
        Content content;
        Lua::WeakFunctionRef acceptedCallback;
        Lua::WeakFunctionRef deniedCallback;
    };

    void screenshotReady(std::string temporaryPath);
    std::filesystem::path galleryPath() const;
    shared_ptr<Capture> captureForPath(const std::filesystem::path& path) const;
    bool isGalleryFile(const std::filesystem::path& path) const;
    std::filesystem::path capturePath(const Reflection::Variant& value) const;
    shared_ptr<Capture> saveCaptureToGallery(const Reflection::Variant& value);
    void publishSavedCapture(const shared_ptr<Capture>& capture,
        const std::string& triggerSource);
    std::filesystem::path externalCapturePath(
        const std::filesystem::path& source) const;
    void invokeCallback(Lua::WeakFunctionRef callback,
        const Reflection::Tuple& arguments);
    void releaseSavePrompt(SavePrompt& prompt);

    rbx::signals::scoped_connection screenshotReadyConnection;
    bool screenshotPending;
    std::string pendingAdditionalInfo;
    long long nextPromptId;
    std::map<long long, SavePrompt> savePrompts;
    std::map<long long, SharePrompt> sharePrompts;
};

} // namespace RBX
