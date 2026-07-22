#include "v8datamodel/SafetyService.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/RbxAnalyticsService.h"

#include <stdexcept>

namespace RBX {

const char* const sSafetyService = "SafetyService";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<SafetyService, bool> propIsCaptureModeForReport(
    "IsCaptureModeForReport", category_Data,
    &SafetyService::getIsCaptureModeForReport,
    &SafetyService::setIsCaptureModeForReport,
    Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);

static Reflection::BoundFuncDesc<SafetyService,
    shared_ptr<const Reflection::ValueTable>(std::string)> funcDecodeAvatarMovementProto(
        &SafetyService::decodeAvatarMovementProto, "DecodeAvatarMovementProto",
        "avatarMovementProtoString", Security::RobloxScript);
static Reflection::BoundFuncDesc<SafetyService,
    long long(shared_ptr<const Reflection::ValueTable>)> funcTakeScreenshot(
        &SafetyService::takeScreenshot, "TakeScreenshot", "screenshotOptions",
        Security::RobloxScript);

#define SAFETY_REPORT_FUNCTION(method, reflectedName) \
    static Reflection::BoundFuncDesc<SafetyService, void()> func##method( \
        &SafetyService::method, reflectedName, Security::RobloxScript)
SAFETY_REPORT_FUNCTION(reportCapturesUIClose, "ReportCapturesUIClose");
SAFETY_REPORT_FUNCTION(reportCapturesUIOpen, "ReportCapturesUIOpen");
SAFETY_REPORT_FUNCTION(reportChatLineReportingClose, "ReportChatLineReportingClose");
SAFETY_REPORT_FUNCTION(reportChatLineReportingOpen, "ReportChatLineReportingOpen");
SAFETY_REPORT_FUNCTION(reportChatSuspensionDialogClose, "ReportChatSuspensionDialogClose");
SAFETY_REPORT_FUNCTION(reportChatSuspensionDialogOpen, "ReportChatSuspensionDialogOpen");
SAFETY_REPORT_FUNCTION(reportMenuTabClose, "ReportMenuTabClose");
SAFETY_REPORT_FUNCTION(reportMenuTabOpen, "ReportMenuTabOpen");
SAFETY_REPORT_FUNCTION(reportPartyChatWindowClose, "ReportPartyChatWindowClose");
SAFETY_REPORT_FUNCTION(reportPartyChatWindowOpen, "ReportPartyChatWindowOpen");
#undef SAFETY_REPORT_FUNCTION

static Reflection::EventDesc<SafetyService, void(long long, ContentId)>
    eventScreenshotContentReady(&SafetyService::screenshotContentReadySignal,
        "ScreenshotContentReady", "screenshotJobId", "contentId", Security::None);
static Reflection::EventDesc<SafetyService, void(long long, std::string)>
    eventScreenshotUploaded(&SafetyService::screenshotUploadedSignal,
        "ScreenshotUploaded", "screenshotJobId", "screenshotId", Security::None);
REFLECTION_END();

SafetyService::SafetyService()
    : Service(true)
    , isCaptureModeForReport(false)
    , nextScreenshotJobId(1)
    , screenshotInProgress(false)
{
    setName(sSafetyService);
    setRobloxLocked(true);
}

void SafetyService::onServiceProvider(ServiceProvider* oldProvider,
    ServiceProvider* newProvider)
{
    screenshotReadyConnection.disconnect();
    screenshotJobs.clear();
    screenshotInProgress = false;
    Super::onServiceProvider(oldProvider, newProvider);

    if (DataModel* dataModel = dynamic_cast<DataModel*>(newProvider))
        screenshotReadyConnection = dataModel->screenshotReadySignal.connect(
            boost::bind(&SafetyService::screenshotReady, this, _1));
}

void SafetyService::setIsCaptureModeForReport(bool value)
{
    if (isCaptureModeForReport == value)
        return;
    isCaptureModeForReport = value;
    raisePropertyChanged(propIsCaptureModeForReport);
    reportSurfaceState("ExperienceStateCapture", value);
}

shared_ptr<const Reflection::ValueTable> SafetyService::decodeAvatarMovementProto(
    std::string)
{
    throw std::runtime_error(
        "DecodeAvatarMovementProto is not supported on client builds");
}

static bool screenshotBooleanOption(
    const shared_ptr<const Reflection::ValueTable>& options,
    const char* name, bool defaultValue)
{
    if (!options)
        return defaultValue;
    Reflection::ValueTable::const_iterator it = options->find(name);
    if (it == options->end())
        return defaultValue;
    if (it->second.isType<bool>())
        return it->second.cast<bool>();
    if (it->second.isType<int>())
        return it->second.cast<int>() != 0;
    throw std::runtime_error("SafetyService screenshot option must be boolean");
}

static std::string screenshotStringOption(
    const shared_ptr<const Reflection::ValueTable>& options, const char* name)
{
    if (!options)
        return std::string();
    Reflection::ValueTable::const_iterator it = options->find(name);
    if (it == options->end())
        return std::string();
    if (!it->second.isType<std::string>())
        throw std::runtime_error("SafetyService screenshot option must be a string");
    return it->second.cast<std::string>();
}

long long SafetyService::takeScreenshot(
    shared_ptr<const Reflection::ValueTable> options)
{
    if (!DataModel::get(this))
        throw std::runtime_error("SafetyService is not attached to a DataModel");

    ScreenshotJob job;
    job.id = nextScreenshotJobId++;
    job.registerContent = screenshotBooleanOption(options, "registerContent", false);
    job.reason = screenshotStringOption(options, "reason");
    screenshotJobs.push_back(job);
    beginNextScreenshot();
    return job.id;
}

void SafetyService::beginNextScreenshot()
{
    if (screenshotInProgress || screenshotJobs.empty())
        return;
    DataModel* dataModel = DataModel::get(this);
    if (!dataModel)
        return;
    screenshotInProgress = true;
    DataModel::TakeScreenshotTask(weak_ptr<DataModel>(shared_from(dataModel)));
}

void SafetyService::screenshotReady(std::string path)
{
    if (!screenshotInProgress || screenshotJobs.empty())
        return;

    const ScreenshotJob job = screenshotJobs.front();
    screenshotJobs.pop_front();
    screenshotInProgress = false;

    screenshotContentReadySignal(job.id, ContentId(std::string("file://") + path));
    beginNextScreenshot();
}

void SafetyService::reportSurfaceState(const char* surface, bool open)
{
    RbxAnalyticsService* analytics = ServiceProvider::find<RbxAnalyticsService>(this);
    if (!analytics)
        return;
    shared_ptr<Reflection::ValueTable> args(new Reflection::ValueTable());
    (*args)["surface"] = Reflection::Variant(std::string(surface));
    (*args)["isOpen"] = Reflection::Variant(open);
    analytics->sendEventDeferred("client", "SafetyService",
        open ? "SurfaceOpened" : "SurfaceClosed", args);
}

#define SAFETY_REPORT_IMPL(method, surface, open) \
    void SafetyService::method() { reportSurfaceState(surface, open); }
SAFETY_REPORT_IMPL(reportCapturesUIClose, "CapturesUI", false)
SAFETY_REPORT_IMPL(reportCapturesUIOpen, "CapturesUI", true)
SAFETY_REPORT_IMPL(reportChatLineReportingClose, "ChatLineReporting", false)
SAFETY_REPORT_IMPL(reportChatLineReportingOpen, "ChatLineReporting", true)
SAFETY_REPORT_IMPL(reportChatSuspensionDialogClose, "ChatSuspensionDialog", false)
SAFETY_REPORT_IMPL(reportChatSuspensionDialogOpen, "ChatSuspensionDialog", true)
SAFETY_REPORT_IMPL(reportMenuTabClose, "MenuTab", false)
SAFETY_REPORT_IMPL(reportMenuTabOpen, "MenuTab", true)
SAFETY_REPORT_IMPL(reportPartyChatWindowClose, "PartyChatWindow", false)
SAFETY_REPORT_IMPL(reportPartyChatWindowOpen, "PartyChatWindow", true)
#undef SAFETY_REPORT_IMPL

} // namespace RBX
