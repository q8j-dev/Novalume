#pragma once

#include "V8Tree/Service.h"

namespace RBX {

extern const char* const sMicroProfilerService;

class MicroProfilerService
    : public DescribedNonCreatable<MicroProfilerService, Instance, sMicroProfilerService>
    , public Service
{
public:
    MicroProfilerService();

    std::string getContextLabel() const { return contextLabel; }
    void setContextLabel(std::string value);
    void dumpToFileAsync(int secondsToDelay, int framesToDump,
        boost::function<void(std::string)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    rbx::signal<void(int, int)> dataChangedSignal;

private:
    std::string contextLabel;
};

} // namespace RBX
