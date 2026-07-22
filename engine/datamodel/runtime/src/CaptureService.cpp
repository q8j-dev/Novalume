#include "v8datamodel/CaptureService.h"

#include "util/FileSystem.h"
#include "Script/LuaArguments.h"
#include "Script/ScriptContext.h"
#include "v8datamodel/DataModel.h"
#include "lua/lua.hpp"
#include "util/standardout.h"

#include <boost/filesystem.hpp>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace RBX {

const char* const sCapture = "Capture";
const char* const sCaptureService = "CaptureService";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<Capture, DateTime> propCaptureTime(
    "CaptureTime", category_Data, &Capture::getCaptureTime, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::EnumPropDescriptor<Capture, Enums::CaptureType> propCaptureType(
    "CaptureType", category_Data, &Capture::getCaptureType, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<Capture, std::string> propFilePathString(
    "FilePathString", category_Data, &Capture::getFilePathString, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<Capture, std::string> propLocalId(
    "LocalId", category_Data, &Capture::getLocalId, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<Capture, long long> propSourcePlaceId(
    "SourcePlaceId", category_Data, &Capture::getSourcePlaceId, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);
static Reflection::PropDescriptor<Capture, long long> propSourceUniverseId(
    "SourceUniverseId", category_Data, &Capture::getSourceUniverseId, NULL,
    Reflection::PropertyDescriptor::SCRIPTING);

static Reflection::BoundFuncDesc<CaptureService, bool()> funcCanCaptureVideo(
    &CaptureService::canCaptureVideo, "CanCaptureVideo", Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, bool()> funcIsCapturingVideo(
    &CaptureService::isCapturingVideo, "IsCapturingVideo", Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, void()> funcOnCaptureBegan(
    &CaptureService::onCaptureBegan, "OnCaptureBegan", Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, void()> funcOnCaptureEnded(
    &CaptureService::onCaptureEnded, "OnCaptureEnded", Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, void(std::string)>
    funcSaveScreenshotCapture(&CaptureService::saveScreenshotCapture,
        "SaveScreenshotCapture", "additionalInfo", std::string(),
        Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService,
    shared_ptr<const Reflection::ValueArray>()> funcRetrieveCaptures(
        &CaptureService::retrieveCaptures, "RetrieveCaptures",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, shared_ptr<Reflection::DescribedBase>(std::string)>
    funcGetScreenshotCaptureObject(&CaptureService::getScreenshotCaptureObject,
        "GetScreenshotCaptureObject", "capturePath", Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, void(std::string)>
    funcDeleteCapture(&CaptureService::deleteCapture, "DeleteCapture",
        "capturePath", Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, std::string(shared_ptr<Reflection::DescribedBase>)>
    funcPreCaptureShared(&CaptureService::preCaptureShared, "PreCaptureShared",
        "capture", Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, void(shared_ptr<Reflection::DescribedBase>)>
    funcOnCaptureObjectShared(&CaptureService::onCaptureObjectShared,
        "OnCaptureObjectShared", "capture", Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, void(std::string)>
    funcOnCaptureShared(&CaptureService::onCaptureShared, "OnCaptureShared",
        "capturePath", Security::RobloxScript);
static Reflection::CustomBoundFuncDesc<CaptureService,
    void(shared_ptr<const Reflection::ValueArray>, Lua::WeakFunctionRef)>
    funcPromptSaveCapturesToGallery(
        &CaptureService::promptSaveCapturesToGallery,
        "PromptSaveCapturesToGallery", "captures", "resultCallback",
        Security::None);
static Reflection::CustomBoundFuncDesc<CaptureService,
    void(long long, shared_ptr<const Reflection::ValueTable>)>
    funcOnSavePromptFinished(&CaptureService::onSavePromptFinished,
        "OnSavePromptFinished", "promptId", "results",
        Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService,
    void(Content, std::string, Lua::WeakFunctionRef, Lua::WeakFunctionRef)>
    funcPromptShareCapture(&CaptureService::promptShareCapture,
        "PromptShareCapture", "captureContent", "launchData",
        "onAcceptedCallback", "onDeniedCallback", Security::None);
static Reflection::BoundFuncDesc<CaptureService, void(long long, bool)>
    funcOnSharePromptFinished(&CaptureService::onSharePromptFinished,
        "OnSharePromptFinished", "promptId", "accepted",
        Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<CaptureService, std::string(Content)>
    funcGetCaptureFilePathAsync(&CaptureService::getCaptureFilePathAsync,
        "GetCaptureFilePathAsync", "captureContent", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<CaptureService,
    long long(shared_ptr<const Reflection::ValueArray>)>
    funcSaveCapturesToExternalStorageAsync(
        &CaptureService::saveCapturesToExternalStorageAsync,
        "SaveCapturesToExternalStorageAsync", "pathArr",
        Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<CaptureService, Vector2(Content)>
    funcGetCaptureSizeAsync(&CaptureService::getCaptureSizeAsync,
        "GetCaptureSizeAsync", "captureContent", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<CaptureService,
    long long(shared_ptr<const Reflection::ValueArray>)>
    funcGetCaptureStorageSizeAsync(&CaptureService::getCaptureStorageSizeAsync,
        "GetCaptureStorageSizeAsync", "pathArr", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<CaptureService,
    long long(shared_ptr<const Reflection::ValueArray>)>
    funcDeleteCapturesAsync(&CaptureService::deleteCapturesAsync,
        "DeleteCapturesAsync", "pathArr", Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<CaptureService,
    Enums::VideoCaptureStartedResult()>
    funcStartVideoCaptureInternalAsync(
        &CaptureService::startVideoCaptureInternalAsync,
        "StartVideoCaptureInternalAsync", Security::RobloxScript);
static Reflection::BoundFuncDesc<CaptureService, void()>
    funcStopVideoCaptureInternal(&CaptureService::stopVideoCaptureInternal,
        "StopVideoCaptureInternal", Security::RobloxScript);

static Reflection::EventDesc<CaptureService, void(Enums::CaptureType)>
    eventCaptureBegan(&CaptureService::captureBeganSignal, "CaptureBegan",
        "captureType", Security::None);
static Reflection::EventDesc<CaptureService, void(Enums::CaptureType)>
    eventCaptureEnded(&CaptureService::captureEndedSignal, "CaptureEnded",
        "captureType", Security::None);
static Reflection::EventDesc<CaptureService,
    void(shared_ptr<const Reflection::ValueTable>, std::string)>
    eventCaptureSavedInternal(&CaptureService::captureSavedInternalSignal,
        "CaptureSavedInternal", "captureInfo", "triggerSource",
        Security::RobloxScript);
static Reflection::EventDesc<CaptureService, void(shared_ptr<Reflection::DescribedBase>, std::string)>
    eventCaptureObjectSavedInternal(
        &CaptureService::captureObjectSavedInternalSignal,
        "CaptureObjectSavedInternal", "capture", "triggerSource",
        Security::RobloxScript);
static Reflection::EventDesc<CaptureService, void(bool, std::string)>
    eventVideoCaptureInProgress(&CaptureService::videoCaptureInProgressSignal,
        "VideoCaptureInProgress", "isInProgress", "captureTrigger",
        Security::RobloxScript);
static Reflection::EventDesc<CaptureService, void(Enums::VideoCaptureResult)>
    eventUserVideoCaptureFailed(&CaptureService::userVideoCaptureFailedSignal,
        "UserVideoCaptureFailed", "result", Security::RobloxScript);
static Reflection::EventDesc<CaptureService,
    void(Enums::VideoCaptureStartedResult)>
    eventUserVideoCaptureStartFailed(
        &CaptureService::userVideoCaptureStartFailedSignal,
        "UserVideoCaptureStartFailed", "result", Security::RobloxScript);
static Reflection::EventDesc<CaptureService,
    void(long long, shared_ptr<const Reflection::ValueArray>)>
    eventOpenSaveCapturesPrompt(
        &CaptureService::openSaveCapturesPromptSignal,
        "OpenSaveCapturesPrompt", "promptId", "captures",
        Security::RobloxScript);
static Reflection::EventDesc<CaptureService, void(long long, Content, std::string)>
    eventOpenShareCapturePrompt(&CaptureService::openShareCapturePromptSignal,
        "OpenShareCapturePrompt", "promptId", "contentId", "launchData",
        Security::RobloxScript);
static Reflection::EventDesc<CaptureService, void(Content)>
    eventUserCaptureSaved(&CaptureService::userCaptureSavedSignal,
        "UserCaptureSaved", "captureContentId", Security::None);
REFLECTION_END();

Capture::Capture(std::filesystem::path filePath, DateTime captureTime,
    Enums::CaptureType captureType, std::string localId,
    long long sourcePlaceId, long long sourceUniverseId)
    : filePath(std::move(filePath))
    , captureTime(captureTime)
    , captureType(captureType)
    , localId(std::move(localId))
    , sourcePlaceId(sourcePlaceId)
    , sourceUniverseId(sourceUniverseId)
{
}

CaptureService::CaptureService()
    : Service(true)
    , screenshotPending(false)
    , nextPromptId(1)
{
    setName(sCaptureService);
    setRobloxLocked(true);
}

std::filesystem::path CaptureService::galleryPath() const
{
    return FileSystem::getUserDirectory(true, DirAppData, "Captures").string();
}

void CaptureService::onServiceProvider(ServiceProvider* oldProvider,
    ServiceProvider* newProvider)
{
    screenshotReadyConnection.disconnect();
    if (oldProvider && !newProvider)
    {
        for (std::map<long long, SavePrompt>::value_type& entry : savePrompts)
            releaseSavePrompt(entry.second);
        savePrompts.clear();
        sharePrompts.clear();
        screenshotPending = false;
        pendingAdditionalInfo.clear();
    }
    Super::onServiceProvider(oldProvider, newProvider);
    if (DataModel* dataModel = dynamic_cast<DataModel*>(newProvider))
        screenshotReadyConnection = dataModel->screenshotReadySignal.connect(
            boost::bind(&CaptureService::screenshotReady, this, _1));
}

void CaptureService::onCaptureBegan()
{
    captureBeganSignal(Enums::CAPTURE_TYPE_SCREENSHOT);
}

void CaptureService::onCaptureEnded()
{
    captureEndedSignal(Enums::CAPTURE_TYPE_SCREENSHOT);
}

void CaptureService::saveScreenshotCapture(std::string additionalInfo)
{
    if (screenshotPending)
        throw runtime_error("CaptureService already has a screenshot in progress");
    DataModel* dataModel = DataModel::get(this);
    if (!dataModel)
        throw runtime_error("CaptureService is not attached to a DataModel");
    screenshotPending = true;
    pendingAdditionalInfo = std::move(additionalInfo);
    DataModel::TakeScreenshotTask(weak_ptr<DataModel>(shared_from(dataModel)));
}

static std::int64_t captureTimestamp(const std::filesystem::path& path)
{
    boost::system::error_code error;
    const std::time_t value = boost::filesystem::last_write_time(path.string(), error);
    if (!error)
        return static_cast<std::int64_t>(value) * 1000;
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

shared_ptr<Capture> CaptureService::captureForPath(
    const std::filesystem::path& path) const
{
    const DataModel* dataModel = DataModel::get(this);
    const long long placeId = dataModel ? dataModel->getPlaceID() : 0;
    const long long universeId = dataModel ? dataModel->getGameId() : 0;
    return shared_ptr<Capture>(new Capture(path, DateTime(captureTimestamp(path)),
        Enums::CAPTURE_TYPE_SCREENSHOT, path.stem().string(), placeId,
        universeId));
}

void CaptureService::screenshotReady(std::string temporaryPath)
{
    if (!screenshotPending)
        return;
    screenshotPending = false;

    const std::filesystem::path source(temporaryPath);
    std::filesystem::path destination = galleryPath() / source.filename();
    std::error_code error;
    std::filesystem::create_directories(destination.parent_path(), error);
    std::filesystem::rename(source, destination, error);
    if (error)
    {
        error.clear();
        std::filesystem::copy_file(source, destination,
            std::filesystem::copy_options::overwrite_existing, error);
        if (!error)
            std::filesystem::remove(source, error);
    }
    if (error)
        throw runtime_error("CaptureService could not store screenshot: %s",
            error.message().c_str());

    shared_ptr<Capture> capture = captureForPath(destination);
    publishSavedCapture(capture, "UserSave");
}

void CaptureService::publishSavedCapture(const shared_ptr<Capture>& capture,
    const std::string& triggerSource)
{
    if (!capture)
        return;
    shared_ptr<Reflection::ValueTable> info(new Reflection::ValueTable());
    (*info)["type"] = Reflection::Variant(std::string("Screenshot"));
    (*info)["filePath"] = Reflection::Variant(capture->getFilePathString());
    (*info)["contentId"] = Reflection::Variant(
        std::string("file://") + capture->getFilePathString());
    if (!pendingAdditionalInfo.empty())
        (*info)["additionalInfo"] = Reflection::Variant(pendingAdditionalInfo);
    pendingAdditionalInfo.clear();

    captureSavedInternalSignal(info, triggerSource);
    captureObjectSavedInternalSignal(capture, triggerSource);
    userCaptureSavedSignal(Content::fromUri(
        std::string("file://") + capture->getFilePathString()));
}

shared_ptr<const Reflection::ValueArray> CaptureService::retrieveCaptures()
{
    shared_ptr<Reflection::ValueArray> captures(new Reflection::ValueArray());
    const std::filesystem::path root = galleryPath();
    std::error_code error;
    if (!std::filesystem::is_directory(root, error))
        return captures;

    std::vector<std::filesystem::path> paths;
    for (std::filesystem::directory_iterator it(root, error), end;
         !error && it != end; it.increment(error))
    {
        if (it->is_regular_file() && it->path().extension() == ".png")
            paths.push_back(it->path());
    }
    std::sort(paths.begin(), paths.end(), std::greater<std::filesystem::path>());
    for (const std::filesystem::path& path : paths)
    {
        shared_ptr<Reflection::ValueTable> info(new Reflection::ValueTable());
        (*info)["type"] = Reflection::Variant(std::string("Screenshot"));
        (*info)["filePath"] = Reflection::Variant(path.string());
        (*info)["contentId"] = Reflection::Variant(
            std::string("file://") + path.string());
        captures->push_back(Reflection::Variant(
            shared_ptr<const Reflection::ValueTable>(info)));
    }
    return captures;
}

bool CaptureService::isGalleryFile(const std::filesystem::path& path) const
{
    std::error_code error;
    const std::filesystem::path root = std::filesystem::weakly_canonical(
        galleryPath(), error);
    if (error)
        return false;
    const std::filesystem::path candidate = std::filesystem::weakly_canonical(
        path, error);
    return !error && candidate.parent_path() == root;
}

shared_ptr<Reflection::DescribedBase> CaptureService::getScreenshotCaptureObject(
    std::string capturePath)
{
    const std::filesystem::path path(capturePath);
    if (!isGalleryFile(path) || !std::filesystem::is_regular_file(path))
        throw runtime_error("CaptureService capture path is not in the gallery");
    return captureForPath(path);
}

void CaptureService::deleteCapture(std::string capturePath)
{
    const std::filesystem::path path(capturePath);
    if (!isGalleryFile(path))
        throw runtime_error("CaptureService capture path is not in the gallery");
    std::error_code error;
    if (!std::filesystem::remove(path, error) || error)
        throw runtime_error("CaptureService could not delete capture");
}

std::string CaptureService::preCaptureShared(shared_ptr<Reflection::DescribedBase> value)
{
    shared_ptr<Capture> capture =
        Reflection::DescribedBase::fastSharedDynamicCast<Capture>(value);
    if (!capture || !isGalleryFile(capture->getFilePath()))
        throw runtime_error("CaptureService expected a gallery Capture");
    return capture->getFilePath().string();
}

void CaptureService::onCaptureObjectShared(shared_ptr<Reflection::DescribedBase> value)
{
    shared_ptr<Capture> capture =
        Reflection::DescribedBase::fastSharedDynamicCast<Capture>(value);
    if (!capture || !isGalleryFile(capture->getFilePath()))
        throw runtime_error("CaptureService expected a gallery Capture");
}

void CaptureService::onCaptureShared(std::string capturePath)
{
    if (!isGalleryFile(capturePath))
        throw runtime_error("CaptureService expected a gallery capture path");
}

static std::filesystem::path pathFromContent(const Content& content)
{
    if (content.getSourceType() == CONTENT_SOURCE_OBJECT)
    {
        shared_ptr<Capture> capture =
            Reflection::DescribedBase::fastSharedDynamicCast<Capture>(
                content.getObject());
        return capture ? capture->getFilePath() : std::filesystem::path();
    }
    if (content.getSourceType() == CONTENT_SOURCE_URI)
    {
        std::string uri = content.getUri();
        if (uri.compare(0, 7, "file://") == 0)
            uri.erase(0, 7);
        return uri;
    }
    return std::filesystem::path();
}

std::filesystem::path CaptureService::capturePath(
    const Reflection::Variant& value) const
{
    if (value.isType<Content>())
        return pathFromContent(value.cast<Content>());
    if (value.isType<std::string>())
        return value.cast<std::string>();
    if (value.isType<shared_ptr<Reflection::DescribedBase> >())
    {
        shared_ptr<Capture> capture =
            Reflection::DescribedBase::fastSharedDynamicCast<Capture>(
                value.cast<shared_ptr<Reflection::DescribedBase> >());
        if (capture)
            return capture->getFilePath();
    }
    return std::filesystem::path();
}

static std::filesystem::path unusedCapturePath(
    const std::filesystem::path& directory,
    const std::filesystem::path& source)
{
    std::filesystem::path filename = source.filename();
    if (filename.empty())
        filename = "RobloxCapture.png";
    std::filesystem::path destination = directory / filename;
    for (unsigned suffix = 1; std::filesystem::exists(destination); ++suffix)
    {
        destination = directory /
            (filename.stem().string() + " (" + std::to_string(suffix) + ")" +
                filename.extension().string());
    }
    return destination;
}

shared_ptr<Capture> CaptureService::saveCaptureToGallery(
    const Reflection::Variant& value)
{
    const std::filesystem::path source = capturePath(value);
    std::error_code error;
    if (source.empty() || !std::filesystem::is_regular_file(source, error) || error)
        throw runtime_error("CaptureService: Invalid content");

    if (isGalleryFile(source))
        return captureForPath(source);

    const std::filesystem::path root = galleryPath();
    std::filesystem::create_directories(root, error);
    if (error)
        throw runtime_error("CaptureService could not create gallery: %s",
            error.message().c_str());
    const std::filesystem::path destination = unusedCapturePath(root, source);
    std::filesystem::copy_file(source, destination,
        std::filesystem::copy_options::none, error);
    if (error)
        throw runtime_error("CaptureService could not save capture: %s",
            error.message().c_str());

    shared_ptr<Capture> capture = captureForPath(destination);
    publishSavedCapture(capture, "PromptSave");
    return capture;
}

void CaptureService::invokeCallback(Lua::WeakFunctionRef callback,
    const Reflection::Tuple& arguments)
{
    if (!callback.lock())
        return;
    try
    {
        ServiceProvider::create<ScriptContext>(this)->callInNewThread(
            callback, arguments);
    }
    catch (const base_exception& error)
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR,
            "CaptureService callback failed: %s", error.what());
    }
}

int CaptureService::promptSaveCapturesToGallery(lua_State* state)
{
    if (lua_gettop(state) != 3 || !lua_istable(state, 2) ||
        !lua_isfunction(state, 3))
        throw runtime_error("PromptSaveCapturesToGallery: Invalid contents");
    const int captureCount = static_cast<int>(lua_objlen(state, 2));
    if (captureCount <= 0)
        throw runtime_error("PromptSaveCapturesToGallery: Invalid contents");

    shared_ptr<Reflection::ValueArray> captures(new Reflection::ValueArray());
    captures->reserve(captureCount);
    SavePrompt prompt;
    prompt.callback = Lua::lua_tofunction(state, 3);
    prompt.originalCaptureRefs.reserve(captureCount);
    for (int index = 1; index <= captureCount; ++index)
    {
        lua_rawgeti(state, 2, index);
        Reflection::Variant value;
        if (!Lua::LuaArguments::get(state, -1, value, false))
        {
            lua_pop(state, 1);
            releaseSavePrompt(prompt);
            throw runtime_error("PromptSaveCapturesToGallery: Invalid content");
        }
        const std::filesystem::path path = capturePath(value);
        std::error_code error;
        if (path.empty() || !std::filesystem::is_regular_file(path, error) || error)
        {
            lua_pop(state, 1);
            releaseSavePrompt(prompt);
            throw runtime_error("PromptSaveCapturesToGallery: Invalid content");
        }
        captures->push_back(value);
        prompt.originalCaptureRefs.push_back(luaL_ref(state, LUA_REGISTRYINDEX));
    }

    const long long promptId = nextPromptId++;
    prompt.captures = captures;
    savePrompts.insert(std::make_pair(promptId, prompt));
    openSaveCapturesPromptSignal(promptId, captures);
    return 0;
}

void CaptureService::releaseSavePrompt(SavePrompt& prompt)
{
    lua_State* state = prompt.callback.threadDangerous();
    if (state)
    {
        for (int reference : prompt.originalCaptureRefs)
            luaL_unref(state, LUA_REGISTRYINDEX, reference);
    }
    prompt.originalCaptureRefs.clear();
}

static bool sameCaptureValue(const Reflection::Variant& left,
    const Reflection::Variant& right)
{
    if (left.isType<Content>() && right.isType<Content>())
        return left.cast<Content>() == right.cast<Content>();
    if (left.isType<shared_ptr<Reflection::DescribedBase> >() &&
        right.isType<shared_ptr<Reflection::DescribedBase> >())
        return left.cast<shared_ptr<Reflection::DescribedBase> >() ==
            right.cast<shared_ptr<Reflection::DescribedBase> >();
    if (left.isType<std::string>() && right.isType<std::string>())
        return left.cast<std::string>() == right.cast<std::string>();
    return false;
}

int CaptureService::onSavePromptFinished(lua_State* state)
{
    if (lua_gettop(state) != 3)
        throw runtime_error("CaptureService:OnSavePromptFinished expects 2 arguments");
    if (!lua_isnumber(state, 2) || !lua_istable(state, 3))
        throw runtime_error("CaptureService: Invalid results");

    const double numericPromptId = lua_tonumber(state, 2);
    const long long promptId = static_cast<long long>(numericPromptId);
    if (static_cast<double>(promptId) != numericPromptId)
        throw runtime_error("CaptureService: Invalid promptId");
    std::map<long long, SavePrompt>::iterator found = savePrompts.find(promptId);
    if (found == savePrompts.end())
        throw runtime_error("CaptureService: Invalid promptId: %lld", promptId);

    std::vector<bool> accepted(found->second.captures->size(), false);
    std::vector<bool> present(found->second.captures->size(), false);
    lua_pushnil(state);
    while (lua_next(state, 3) != 0)
    {
        if (!lua_isboolean(state, -1))
        {
            lua_pop(state, 2);
            throw runtime_error("CaptureService: Invalid results");
        }
        Reflection::Variant returnedCapture;
        if (!Lua::LuaArguments::get(state, -2, returnedCapture, false))
        {
            lua_pop(state, 2);
            throw runtime_error("CaptureService: Invalid results");
        }
        std::size_t matched = found->second.captures->size();
        for (std::size_t index = 0; index < found->second.captures->size(); ++index)
        {
            if (!present[index] && sameCaptureValue(
                    returnedCapture, (*found->second.captures)[index]))
            {
                matched = index;
                break;
            }
        }
        if (matched == found->second.captures->size())
        {
            lua_pop(state, 2);
            throw runtime_error("CaptureService: Invalid results");
        }
        present[matched] = true;
        accepted[matched] = lua_toboolean(state, -1) != 0;
        lua_pop(state, 1);
    }
    if (std::find(present.begin(), present.end(), false) != present.end())
        throw runtime_error("CaptureService: Invalid results");

    for (std::size_t index = 0; index < accepted.size(); ++index)
    {
        if (accepted[index])
            saveCaptureToGallery((*found->second.captures)[index]);
    }

    SavePrompt completedPrompt = found->second;
    savePrompts.erase(found);
    if (completedPrompt.callback.lock())
    {
        Lua::lua_pushfunction(state, completedPrompt.callback);
        lua_createtable(state, 0,
            static_cast<int>(completedPrompt.originalCaptureRefs.size()));
        for (std::size_t index = 0;
             index < completedPrompt.originalCaptureRefs.size(); ++index)
        {
            lua_rawgeti(state, LUA_REGISTRYINDEX,
                completedPrompt.originalCaptureRefs[index]);
            lua_pushboolean(state, accepted[index]);
            lua_settable(state, -3);
        }
        if (lua_pcall(state, 1, 0, 0) != 0)
        {
            const char* message = lua_tostring(state, -1);
            const std::string error = message ? message : "unknown callback error";
            lua_pop(state, 1);
            releaseSavePrompt(completedPrompt);
            throw runtime_error("CaptureService callback failed: %s", error.c_str());
        }
    }
    releaseSavePrompt(completedPrompt);
    return 0;
}

void CaptureService::promptShareCapture(Content captureContent,
    std::string launchData, Lua::WeakFunctionRef onAcceptedCallback,
    Lua::WeakFunctionRef onDeniedCallback)
{
    const std::filesystem::path path = pathFromContent(captureContent);
    std::error_code error;
    if (path.empty() || !std::filesystem::is_regular_file(path, error) || error)
        throw runtime_error("PromptShareCapture: Invalid capture Content");
    if (!onAcceptedCallback.lock() || !onDeniedCallback.lock())
        throw runtime_error("PromptShareCapture requires accepted and denied callbacks");

    const long long promptId = nextPromptId++;
    SharePrompt prompt;
    prompt.content = captureContent;
    prompt.acceptedCallback = onAcceptedCallback;
    prompt.deniedCallback = onDeniedCallback;
    sharePrompts.insert(std::make_pair(promptId, prompt));
    openShareCapturePromptSignal(promptId, captureContent, launchData);
}

void CaptureService::onSharePromptFinished(long long promptId, bool accepted)
{
    std::map<long long, SharePrompt>::iterator found = sharePrompts.find(promptId);
    if (found == sharePrompts.end())
        throw runtime_error("CaptureService: Invalid promptId: %lld", promptId);
    Lua::WeakFunctionRef callback = accepted
        ? found->second.acceptedCallback
        : found->second.deniedCallback;
    sharePrompts.erase(found);
    invokeCallback(callback, Reflection::Tuple());
}

void CaptureService::getCaptureFilePathAsync(Content captureContent,
    boost::function<void(std::string)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    const std::filesystem::path path = pathFromContent(captureContent);
    std::error_code error;
    if (path.empty() || !std::filesystem::is_regular_file(path, error) || error)
    {
        errorFunction("Invalid capture Content");
        return;
    }
    resumeFunction(path.string());
}

std::filesystem::path CaptureService::externalCapturePath(
    const std::filesystem::path& source) const
{
    return unusedCapturePath(
        FileSystem::getUserDirectory(true, DirPicture).string(), source);
}

void CaptureService::saveCapturesToExternalStorageAsync(
    shared_ptr<const Reflection::ValueArray> captures,
    boost::function<void(long long)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!captures)
    {
        errorFunction("Invalid Path Array");
        return;
    }

    long long saved = 0;
    for (const Reflection::Variant& value : *captures)
    {
        const std::filesystem::path source = capturePath(value);
        std::error_code error;
        if (source.empty() || !isGalleryFile(source) ||
            !std::filesystem::is_regular_file(source, error) || error)
        {
            errorFunction("SaveCapturesToExternalStorageAsync: Invalid Capture");
            return;
        }
        const std::filesystem::path destination = externalCapturePath(source);
        std::filesystem::copy_file(source, destination,
            std::filesystem::copy_options::none, error);
        if (error)
        {
            errorFunction(error.message());
            return;
        }
        ++saved;
    }
    resumeFunction(saved);
}

void CaptureService::getCaptureSizeAsync(Content captureContent,
    boost::function<void(Vector2)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    const std::filesystem::path path = pathFromContent(captureContent);
    if (!isGalleryFile(path))
    {
        errorFunction("CaptureService expected gallery content");
        return;
    }
    std::ifstream input(path, std::ios::binary);
    unsigned char header[24] = {};
    input.read(reinterpret_cast<char*>(header), sizeof(header));
    if (input.gcount() != sizeof(header) || header[0] != 0x89 ||
        header[1] != 'P' || header[2] != 'N' || header[3] != 'G')
    {
        errorFunction("CaptureService capture is not a valid PNG");
        return;
    }
    const unsigned width = (header[16] << 24) | (header[17] << 16) |
        (header[18] << 8) | header[19];
    const unsigned height = (header[20] << 24) | (header[21] << 16) |
        (header[22] << 8) | header[23];
    resumeFunction(Vector2(static_cast<float>(width), static_cast<float>(height)));
}

static bool valuePath(const Reflection::Variant& value,
    std::filesystem::path& result)
{
    if (value.isType<std::string>())
    {
        result = value.cast<std::string>();
        return true;
    }
    if (value.isType<shared_ptr<Reflection::DescribedBase> >())
    {
        shared_ptr<Reflection::DescribedBase> object =
            value.cast<shared_ptr<Reflection::DescribedBase> >();
        if (shared_ptr<Capture> capture =
                Reflection::DescribedBase::fastSharedDynamicCast<Capture>(object))
        {
            result = capture->getFilePath();
            return true;
        }
    }
    return false;
}

void CaptureService::getCaptureStorageSizeAsync(
    shared_ptr<const Reflection::ValueArray> paths,
    boost::function<void(long long)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!paths)
    {
        errorFunction("CaptureService expected an array of captures");
        return;
    }
    long long total = 0;
    for (const Reflection::Variant& value : *paths)
    {
        std::filesystem::path path;
        if (!valuePath(value, path) || !isGalleryFile(path))
        {
            errorFunction("CaptureService expected gallery captures");
            return;
        }
        std::error_code error;
        total += static_cast<long long>(std::filesystem::file_size(path, error));
        if (error)
        {
            errorFunction(error.message());
            return;
        }
    }
    resumeFunction(total);
}

void CaptureService::deleteCapturesAsync(
    shared_ptr<const Reflection::ValueArray> paths,
    boost::function<void(long long)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!paths)
    {
        errorFunction("CaptureService expected an array of captures");
        return;
    }
    long long deleted = 0;
    for (const Reflection::Variant& value : *paths)
    {
        std::filesystem::path path;
        if (!valuePath(value, path) || !isGalleryFile(path))
        {
            errorFunction("CaptureService expected gallery captures");
            return;
        }
        std::error_code error;
        if (std::filesystem::remove(path, error))
            ++deleted;
        else if (error)
        {
            errorFunction(error.message());
            return;
        }
    }
    resumeFunction(deleted);
}

void CaptureService::startVideoCaptureInternalAsync(
    boost::function<void(Enums::VideoCaptureStartedResult)> resumeFunction,
    boost::function<void(std::string)>)
{
    // The current macOS host has no installed video encoder bridge. Report the
    // exact capability result instead of claiming that a recording started.
    resumeFunction(Enums::VIDEO_CAPTURE_STARTED_NO_DEVICE_SUPPORT);
    userVideoCaptureStartFailedSignal(
        Enums::VIDEO_CAPTURE_STARTED_NO_DEVICE_SUPPORT);
}

void CaptureService::stopVideoCaptureInternal()
{
}

} // namespace RBX
