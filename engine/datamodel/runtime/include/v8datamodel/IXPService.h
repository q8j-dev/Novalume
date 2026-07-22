#pragma once

#include "v8tree/Service.h"

#include <map>
#include <set>
#include <string>

namespace RBX {

extern const char* const sIXPService;

class IXPService
    : public DescribedNonCreatable<IXPService, Instance, sIXPService>
    , public Service
{
public:
    enum IXPLoadingStatus
    {
        IXP_NONE = 0,
        IXP_PENDING = 1,
        IXP_INITIALIZED = 2,
        IXP_ERROR_INVALID_USER = 3,
        IXP_ERROR_CONNECTION = 4,
        IXP_ERROR_JSON_PARSE = 5,
        IXP_ERROR_TIMED_OUT = 6
    };

    IXPService();

    void clearCreatorLayers();
    void clearUserLayers();
    IXPLoadingStatus getBrowserTrackerLayerLoadingStatus();
    shared_ptr<const Reflection::ValueTable> getBrowserTrackerLayerVariables(std::string layer);
    Reflection::Variant getBrowserTrackerStatusForLayer(std::string layer);
    IXPLoadingStatus getCreatorLayerLoadingStatus();
    shared_ptr<const Reflection::ValueTable> getCreatorLayerVariables(std::string layer);
    Reflection::Variant getCreatorStatusForLayer(std::string layer);
    shared_ptr<const Reflection::ValueTable> getRegisteredCreatorLayersToStatus();
    shared_ptr<const Reflection::ValueTable> getRegisteredUserLayersToStatus();
    IXPLoadingStatus getUserLayerLoadingStatus();
    shared_ptr<const Reflection::ValueTable> getUserLayerVariables(std::string layer);
    Reflection::Variant getUserStatusForLayer(std::string layer);
    void initializeCreatorLayers(int creatorId);
    void initializeUserLayers(int userId);
    void logBrowserTrackerLayerExposure(std::string layer);
    void logCreatorLayerExposure(std::string layer);
    void logFlagLinkedUserLayerExposure(std::string layer);
    void logUserLayerExposure(std::string layer);
    void registerCreatorLayers(Reflection::Variant layers);
    void registerUserLayers(Reflection::Variant layers);

    rbx::signal<void(IXPLoadingStatus)> browserTrackerStatusChanged;
    rbx::signal<void(IXPLoadingStatus)> creatorStatusChanged;
    rbx::signal<void(IXPLoadingStatus)> userStatusChanged;

private:
    typedef std::map<std::string, IXPLoadingStatus> LayerStatuses;
    typedef std::map<std::string, shared_ptr<const Reflection::ValueTable> > LayerVariables;

    static void registerLayers(const Reflection::Variant& input, LayerStatuses& statuses,
        LayerVariables& variables);
    static shared_ptr<const Reflection::ValueTable> variablesFor(
        const std::string& layer, const LayerVariables& variables);
    static Reflection::Variant statusFor(const std::string& layer,
        const LayerStatuses& statuses);
    static shared_ptr<const Reflection::ValueTable> statusTable(
        const LayerStatuses& statuses);
    static void setAllStatuses(LayerStatuses& statuses, IXPLoadingStatus status);

    IXPLoadingStatus browserStatus;
    IXPLoadingStatus creatorStatus;
    IXPLoadingStatus userStatus;
    LayerStatuses browserLayers;
    LayerStatuses creatorLayers;
    LayerStatuses userLayers;
    LayerVariables browserVariables;
    LayerVariables creatorVariables;
    LayerVariables userVariables;
    std::set<std::string> exposures;
};

} // namespace RBX
