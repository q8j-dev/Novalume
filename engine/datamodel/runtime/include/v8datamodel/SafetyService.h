#pragma once

#include "util/ContentId.h"
#include "v8tree/Service.h"

#include "rbx/signal.h"

#include <deque>
#include <string>

namespace RBX {

extern const char* const sSafetyService;

class SafetyService
    : public DescribedNonCreatable<SafetyService, Instance, sSafetyService>
    , public Service
{
public:
    typedef DescribedNonCreatable<SafetyService, Instance, sSafetyService> Super;

    SafetyService();

    bool getIsCaptureModeForReport() const { return isCaptureModeForReport; }
    void setIsCaptureModeForReport(bool value);

    shared_ptr<const Reflection::ValueTable> decodeAvatarMovementProto(std::string value);
    long long takeScreenshot(shared_ptr<const Reflection::ValueTable> options);

    void reportCapturesUIClose();
    void reportCapturesUIOpen();
    void reportChatLineReportingClose();
    void reportChatLineReportingOpen();
    void reportChatSuspensionDialogClose();
    void reportChatSuspensionDialogOpen();
    void reportMenuTabClose();
    void reportMenuTabOpen();
    void reportPartyChatWindowClose();
    void reportPartyChatWindowOpen();

    rbx::signal<void(long long, ContentId)> screenshotContentReadySignal;
    rbx::signal<void(long long, std::string)> screenshotUploadedSignal;

protected:
    void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) override;

private:
    struct ScreenshotJob
    {
        long long id;
        bool registerContent;
        std::string reason;
    };

    void reportSurfaceState(const char* surface, bool open);
    void beginNextScreenshot();
    void screenshotReady(std::string path);

    bool isCaptureModeForReport;
    long long nextScreenshotJobId;
    bool screenshotInProgress;
    std::deque<ScreenshotJob> screenshotJobs;
    rbx::signals::scoped_connection screenshotReadyConnection;
};

} // namespace RBX
