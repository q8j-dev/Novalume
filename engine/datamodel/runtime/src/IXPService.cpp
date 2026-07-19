#include "V8DataModel/IXPService.h"

#include "Util/standardout.h"

namespace RBX {

const char* const sIXPService = "IXPService";

namespace Reflection {
template<> EnumDesc<IXPService::IXPLoadingStatus>::EnumDesc()
    : EnumDescriptor("IXPLoadingStatus")
{
    addPair(IXPService::IXP_NONE, "None");
    addPair(IXPService::IXP_PENDING, "Pending");
    addPair(IXPService::IXP_INITIALIZED, "Initialized");
    addPair(IXPService::IXP_ERROR_INVALID_USER, "ErrorInvalidUser");
    addPair(IXPService::IXP_ERROR_CONNECTION, "ErrorConnection");
    addPair(IXPService::IXP_ERROR_JSON_PARSE, "ErrorJsonParse");
    addPair(IXPService::IXP_ERROR_TIMED_OUT, "ErrorTimedOut");
}
template<> IXPService::IXPLoadingStatus& Variant::convert<IXPService::IXPLoadingStatus>()
{
    return genericConvert<IXPService::IXPLoadingStatus>();
}
} // namespace Reflection

template<> bool StringConverter<IXPService::IXPLoadingStatus>::convertToValue(
    const std::string& text, IXPService::IXPLoadingStatus& value)
{
    return Reflection::EnumDesc<IXPService::IXPLoadingStatus>::singleton()
        .convertToValue(text.c_str(), value);
}

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<IXPService, void()> funcClearCreator(
    &IXPService::clearCreatorLayers, "ClearCreatorLayers", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, void()> funcClearUser(
    &IXPService::clearUserLayers, "ClearUserLayers", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, IXPService::IXPLoadingStatus()> funcGetBrowserStatus(
    &IXPService::getBrowserTrackerLayerLoadingStatus,
    "GetBrowserTrackerLayerLoadingStatus", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, shared_ptr<const Reflection::ValueTable>(std::string)> funcGetBrowserVariables(
    &IXPService::getBrowserTrackerLayerVariables, "GetBrowserTrackerLayerVariables", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, Reflection::Variant(std::string)> funcGetBrowserLayerStatus(
    &IXPService::getBrowserTrackerStatusForLayer, "GetBrowserTrackerStatusForLayer", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, IXPService::IXPLoadingStatus()> funcGetCreatorStatus(
    &IXPService::getCreatorLayerLoadingStatus, "GetCreatorLayerLoadingStatus", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, shared_ptr<const Reflection::ValueTable>(std::string)> funcGetCreatorVariables(
    &IXPService::getCreatorLayerVariables, "GetCreatorLayerVariables", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, Reflection::Variant(std::string)> funcGetCreatorLayerStatus(
    &IXPService::getCreatorStatusForLayer, "GetCreatorStatusForLayer", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, shared_ptr<const Reflection::ValueTable>()> funcGetCreatorLayers(
    &IXPService::getRegisteredCreatorLayersToStatus, "GetRegisteredCreatorLayersToStatus", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, shared_ptr<const Reflection::ValueTable>()> funcGetUserLayers(
    &IXPService::getRegisteredUserLayersToStatus, "GetRegisteredUserLayersToStatus", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, IXPService::IXPLoadingStatus()> funcGetUserStatus(
    &IXPService::getUserLayerLoadingStatus, "GetUserLayerLoadingStatus", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, shared_ptr<const Reflection::ValueTable>(std::string)> funcGetUserVariables(
    &IXPService::getUserLayerVariables, "GetUserLayerVariables", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, Reflection::Variant(std::string)> funcGetUserLayerStatus(
    &IXPService::getUserStatusForLayer, "GetUserStatusForLayer", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, void(int)> funcInitializeCreator(
    &IXPService::initializeCreatorLayers, "InitializeCreatorLayers", "creatorId", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, void(int)> funcInitializeUser(
    &IXPService::initializeUserLayers, "InitializeUserLayers", "userId", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, void(std::string)> funcLogBrowser(
    &IXPService::logBrowserTrackerLayerExposure, "LogBrowserTrackerLayerExposure", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, void(std::string)> funcLogCreator(
    &IXPService::logCreatorLayerExposure, "LogCreatorLayerExposure", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, void(std::string)> funcLogFlagLinkedUser(
    &IXPService::logFlagLinkedUserLayerExposure, "LogFlagLinkedUserLayerExposure", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, void(std::string)> funcLogUser(
    &IXPService::logUserLayerExposure, "LogUserLayerExposure", "layerName", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, void(Reflection::Variant)> funcRegisterCreator(
    &IXPService::registerCreatorLayers, "RegisterCreatorLayers", "creatorLayers", Security::LocalUser);
static Reflection::BoundFuncDesc<IXPService, void(Reflection::Variant)> funcRegisterUser(
    &IXPService::registerUserLayers, "RegisterUserLayers", "userLayers", Security::LocalUser);
static Reflection::EventDesc<IXPService, void(IXPService::IXPLoadingStatus)> eventBrowserStatus(
    &IXPService::browserTrackerStatusChanged, "OnBrowserTrackerLayerLoadingStatusChanged", "status", Security::LocalUser);
static Reflection::EventDesc<IXPService, void(IXPService::IXPLoadingStatus)> eventCreatorStatus(
    &IXPService::creatorStatusChanged, "OnCreatorLayerLoadingStatusChanged", "status", Security::LocalUser);
static Reflection::EventDesc<IXPService, void(IXPService::IXPLoadingStatus)> eventUserStatus(
    &IXPService::userStatusChanged, "OnUserLayerLoadingStatusChanged", "status", Security::LocalUser);
REFLECTION_END();

IXPService::IXPService()
    : Service(true), browserStatus(IXP_NONE), creatorStatus(IXP_NONE), userStatus(IXP_NONE)
{
    setName(sIXPService);
    setRobloxLocked(true);
}

void IXPService::registerLayers(const Reflection::Variant& input,
    LayerStatuses& statuses, LayerVariables& variables)
{
    if (input.isType<shared_ptr<const Reflection::ValueArray> >())
    {
        shared_ptr<const Reflection::ValueArray> values = input.cast<shared_ptr<const Reflection::ValueArray> >();
        for (Reflection::ValueArray::const_iterator it = values->begin(); it != values->end(); ++it)
            if (it->isType<std::string>()) statuses[it->cast<std::string>()] = IXP_NONE;
    }
    else if (input.isType<shared_ptr<const Reflection::ValueTable> >())
    {
        shared_ptr<const Reflection::ValueTable> table = input.cast<shared_ptr<const Reflection::ValueTable> >();
        for (Reflection::ValueTable::const_iterator it = table->begin(); it != table->end(); ++it)
        {
            statuses[it->first] = IXP_NONE;
            if (it->second.isType<shared_ptr<const Reflection::ValueTable> >())
                variables[it->first] = it->second.cast<shared_ptr<const Reflection::ValueTable> >();
        }
    }
}

shared_ptr<const Reflection::ValueTable> IXPService::variablesFor(
    const std::string& layer, const LayerVariables& variables)
{
    LayerVariables::const_iterator found = variables.find(layer);
    return found == variables.end()
        ? shared_ptr<const Reflection::ValueTable>(new Reflection::ValueTable())
        : found->second;
}

Reflection::Variant IXPService::statusFor(const std::string& layer,
    const LayerStatuses& statuses)
{
    LayerStatuses::const_iterator found = statuses.find(layer);
    return found == statuses.end() ? Reflection::Variant() : Reflection::Variant(found->second);
}

shared_ptr<const Reflection::ValueTable> IXPService::statusTable(const LayerStatuses& statuses)
{
    shared_ptr<Reflection::ValueTable> result(new Reflection::ValueTable());
    for (LayerStatuses::const_iterator it = statuses.begin(); it != statuses.end(); ++it)
        (*result)[it->first] = Reflection::Variant(it->second);
    return result;
}

void IXPService::setAllStatuses(LayerStatuses& statuses, IXPLoadingStatus status)
{ for (LayerStatuses::iterator it = statuses.begin(); it != statuses.end(); ++it) it->second = status; }

void IXPService::clearCreatorLayers() { creatorLayers.clear(); creatorVariables.clear(); creatorStatus = IXP_NONE; creatorStatusChanged(creatorStatus); }
void IXPService::clearUserLayers() { userLayers.clear(); userVariables.clear(); userStatus = IXP_NONE; userStatusChanged(userStatus); }
IXPService::IXPLoadingStatus IXPService::getBrowserTrackerLayerLoadingStatus() { return browserStatus; }
shared_ptr<const Reflection::ValueTable> IXPService::getBrowserTrackerLayerVariables(std::string layer) { return variablesFor(layer, browserVariables); }
Reflection::Variant IXPService::getBrowserTrackerStatusForLayer(std::string layer) { return statusFor(layer, browserLayers); }
IXPService::IXPLoadingStatus IXPService::getCreatorLayerLoadingStatus() { return creatorStatus; }
shared_ptr<const Reflection::ValueTable> IXPService::getCreatorLayerVariables(std::string layer) { return variablesFor(layer, creatorVariables); }
Reflection::Variant IXPService::getCreatorStatusForLayer(std::string layer) { return statusFor(layer, creatorLayers); }
shared_ptr<const Reflection::ValueTable> IXPService::getRegisteredCreatorLayersToStatus() { return statusTable(creatorLayers); }
shared_ptr<const Reflection::ValueTable> IXPService::getRegisteredUserLayersToStatus() { return statusTable(userLayers); }
IXPService::IXPLoadingStatus IXPService::getUserLayerLoadingStatus() { return userStatus; }
shared_ptr<const Reflection::ValueTable> IXPService::getUserLayerVariables(std::string layer) { return variablesFor(layer, userVariables); }
Reflection::Variant IXPService::getUserStatusForLayer(std::string layer) { return statusFor(layer, userLayers); }

void IXPService::initializeCreatorLayers(int creatorId)
{
    creatorStatus = creatorId < 0 ? IXP_ERROR_INVALID_USER : IXP_PENDING;
    creatorStatusChanged(creatorStatus);
    if (creatorId >= 0) { creatorStatus = IXP_INITIALIZED; setAllStatuses(creatorLayers, creatorStatus); creatorStatusChanged(creatorStatus); }
}
void IXPService::initializeUserLayers(int userId)
{
    userStatus = userId < 0 ? IXP_ERROR_INVALID_USER : IXP_PENDING;
    userStatusChanged(userStatus);
    if (userId >= 0) { userStatus = IXP_INITIALIZED; setAllStatuses(userLayers, userStatus); userStatusChanged(userStatus); }
}

void IXPService::logBrowserTrackerLayerExposure(std::string layer) { exposures.insert("browser:" + layer); }
void IXPService::logCreatorLayerExposure(std::string layer) { exposures.insert("creator:" + layer); }
void IXPService::logFlagLinkedUserLayerExposure(std::string layer) { exposures.insert("flag-user:" + layer); }
void IXPService::logUserLayerExposure(std::string layer) { exposures.insert("user:" + layer); }
void IXPService::registerCreatorLayers(Reflection::Variant layers) { registerLayers(layers, creatorLayers, creatorVariables); }
void IXPService::registerUserLayers(Reflection::Variant layers) { registerLayers(layers, userLayers, userVariables); }

} // namespace RBX
