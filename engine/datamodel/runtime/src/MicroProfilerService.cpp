#include "v8datamodel/MicroProfilerService.h"

#include "v8datamodel/DataModel.h"
#include "util/FileSystem.h"
#include "rbx/Profiler.h"

#include <boost/filesystem.hpp>
#include <boost/thread.hpp>
#include <ctime>
#include <iomanip>
#include <sstream>

namespace RBX {

const char* const sMicroProfilerService = "MicroProfilerService";

FASTINTVARIABLE(MicroProfilerServiceContextLabelMaxLength, 128)

REFLECTION_BEGIN();
static Reflection::PropDescriptor<MicroProfilerService, std::string> propContextLabel(
    "ContextLabel", category_Data, &MicroProfilerService::getContextLabel,
    &MicroProfilerService::setContextLabel, Reflection::PropertyDescriptor::SCRIPTING,
    Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<MicroProfilerService, std::string(int, int)>
    funcDumpToFileAsync(&MicroProfilerService::dumpToFileAsync, "DumpToFileAsync",
        "secondsToDelay", "framesToDump", Security::RobloxScript);
static Reflection::EventDesc<MicroProfilerService, void(int, int)> eventDataChanged(
    &MicroProfilerService::dataChangedSignal, "DataChanged", "slotId", "flags");
REFLECTION_END();

namespace {

std::string makeDumpPath()
{
    std::time_t now = std::time(NULL);
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &now);
#else
    localtime_r(&now, &localTime);
#endif
    std::ostringstream filename;
    filename << "MicroProfilerDump_" << std::put_time(&localTime, "%Y.%m.%d_%H.%M.%S")
             << ".html";
    return (FileSystem::getLogsDirectory() / filename.str()).string();
}

void resumeDump(boost::function<void(std::string)> resumeFunction,
    std::string path, DataModel*)
{
    resumeFunction(path);
}

void failDump(boost::function<void(std::string)> errorFunction,
    std::string message, DataModel*)
{
    errorFunction(message);
}

void performDump(weak_ptr<DataModel> weakDataModel, int secondsToDelay,
    int framesToDump, boost::function<void(std::string)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (secondsToDelay > 0)
        boost::this_thread::sleep(boost::posix_time::seconds(secondsToDelay));

    const std::string path = makeDumpPath();
    Profiler::dumpToFile(path.c_str(), static_cast<unsigned int>(framesToDump));

    const int timeoutChecks = 600;
    for (int check = 0; check < timeoutChecks; ++check)
    {
        boost::system::error_code error;
        if (boost::filesystem::exists(path, error) &&
            boost::filesystem::file_size(path, error) > 0)
        {
            if (shared_ptr<DataModel> dataModel = weakDataModel.lock())
                dataModel->submitTask(boost::bind(&resumeDump, resumeFunction, path, _1),
                    DataModelJob::Write);
            return;
        }
        boost::this_thread::sleep(boost::posix_time::milliseconds(100));
    }

    if (shared_ptr<DataModel> dataModel = weakDataModel.lock())
        dataModel->submitTask(boost::bind(&failDump, errorFunction,
            std::string("MicroProfiler dump timed out after 60 seconds"), _1),
            DataModelJob::Write);
}

} // namespace

MicroProfilerService::MicroProfilerService()
    : Service(true)
{
    setName(sMicroProfilerService);
    setRobloxLocked(true);
    Profiler::enableCapture();
}

void MicroProfilerService::setContextLabel(std::string value)
{
    const size_t limit = static_cast<size_t>(std::max(0,
        FInt::MicroProfilerServiceContextLabelMaxLength));
    if (value.size() > limit)
        value.resize(limit);
    if (contextLabel == value)
        return;
    contextLabel = value;
    raisePropertyChanged(propContextLabel);
}

void MicroProfilerService::dumpToFileAsync(int secondsToDelay, int framesToDump,
    boost::function<void(std::string)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (secondsToDelay < 0)
        throw runtime_error("secondsToDelay must not be negative");
    if (framesToDump <= 0)
        throw runtime_error("framesToDump must be greater than zero");

    DataModel* dataModel = DataModel::get(this);
    if (!dataModel)
        throw runtime_error("MicroProfilerService is not attached to a DataModel");

    boost::thread worker(boost::bind(&performDump, weak_from(dataModel),
        secondsToDelay, framesToDump, resumeFunction, errorFunction));
    worker.detach();
}

} // namespace RBX
