#include "V8DataModel/PolicyService.h"

#include "V8DataModel/LocalizationService.h"
#include "reflection/enumconverter.h"

#include <algorithm>

namespace RBX {

const char* const sPolicyService = "PolicyService";

namespace Reflection {
template<> EnumDesc<PolicyService::TriStateBoolean>::EnumDesc()
    : EnumDescriptor("TriStateBoolean")
{
    addPair(PolicyService::TRI_STATE_UNKNOWN, "Unknown");
    addPair(PolicyService::TRI_STATE_TRUE, "True");
    addPair(PolicyService::TRI_STATE_FALSE, "False");
}
template<> PolicyService::TriStateBoolean&
Variant::convert<PolicyService::TriStateBoolean>()
{
    return genericConvert<PolicyService::TriStateBoolean>();
}
} // namespace Reflection

template<> bool StringConverter<PolicyService::TriStateBoolean>::convertToValue(
    const std::string& text, PolicyService::TriStateBoolean& value)
{
    return Reflection::EnumDesc<PolicyService::TriStateBoolean>::singleton()
        .convertToValue(text.c_str(), value);
}

REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<PolicyService,
    PolicyService::TriStateBoolean> propIsLuobuServer(
        "IsLuobuServer", category_Data, &PolicyService::getIsLuobuServer,
        &PolicyService::setIsLuobuServer, Reflection::PropertyDescriptor::STANDARD,
        Security::RobloxScript);
static Reflection::EnumPropDescriptor<PolicyService,
    PolicyService::TriStateBoolean> propLuobuWhitelisted(
        "LuobuWhitelisted", category_Data, &PolicyService::getLuobuWhitelisted,
        &PolicyService::setLuobuWhitelisted,
        Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
static Reflection::BoundYieldFuncDesc<PolicyService,
    bool(shared_ptr<Instance>, std::string)> funcCanViewBrandProjectAsync(
        &PolicyService::canViewBrandProjectAsync, "CanViewBrandProjectAsync",
        "player", "brandProjectId", Security::None);
static Reflection::BoundYieldFuncDesc<PolicyService,
    shared_ptr<const Reflection::ValueTable>(shared_ptr<Instance>)>
        funcGetPolicyInfoForPlayerAsync(&PolicyService::getPolicyInfoForPlayerAsync,
            "GetPolicyInfoForPlayerAsync", "player", Security::None);
static Reflection::BoundYieldFuncDesc<PolicyService,
    shared_ptr<const Reflection::ValueTable>()>
        funcGetPolicyInfoForServerRobloxOnlyAsync(
            &PolicyService::getPolicyInfoForServerRobloxOnlyAsync,
            "GetPolicyInfoForServerRobloxOnlyAsync", Security::RobloxScript);
REFLECTION_END();

PolicyService::PolicyService()
    : Service(true)
    , isLuobuServer(TRI_STATE_FALSE)
    , luobuWhitelisted(TRI_STATE_UNKNOWN)
{
    setName(sPolicyService);
    setRobloxLocked(true);
}

void PolicyService::setIsLuobuServer(TriStateBoolean value)
{
    if (isLuobuServer == value)
        return;
    isLuobuServer = value;
    raiseChanged(propIsLuobuServer);
}

void PolicyService::setLuobuWhitelisted(TriStateBoolean value)
{
    if (luobuWhitelisted == value)
        return;
    luobuWhitelisted = value;
    raiseChanged(propLuobuWhitelisted);
}

shared_ptr<const Reflection::ValueTable> PolicyService::buildPolicyInfo() const
{
    std::string locale;
    if (const LocalizationService* localization =
            ServiceProvider::find<LocalizationService>(this))
        locale = localization->getSystemLocaleId();
    std::transform(locale.begin(), locale.end(), locale.begin(),
        [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    const bool chinaPolicy = locale == "zh-cn" || locale == "zh_cn" ||
        locale.find("zh-hans") == 0 || isLuobuServer == TRI_STATE_TRUE;

    shared_ptr<Reflection::ValueTable> policy(new Reflection::ValueTable());
    (*policy)["IsSubjectToChinaPolicies"] = Reflection::Variant(chinaPolicy);
    (*policy)["IsContentSharingAllowed"] = Reflection::Variant(!chinaPolicy);
    (*policy)["DefaultAvatarDeathType"] = Reflection::Variant(std::string("Standard"));
    (*policy)["AreAdsAllowed"] = Reflection::Variant(!chinaPolicy);
    (*policy)["ArePaidRandomItemsRestricted"] = Reflection::Variant(false);
    (*policy)["IsPaidItemTradingAllowed"] = Reflection::Variant(!chinaPolicy);
    return policy;
}

void PolicyService::canViewBrandProjectAsync(shared_ptr<Instance> player,
    std::string brandProjectId, boost::function<void(bool)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!player)
    {
        errorFunction("PolicyService expected Player, got nil");
        return;
    }
    resumeFunction(!brandProjectId.empty() && isLuobuServer != TRI_STATE_TRUE);
}

void PolicyService::getPolicyInfoForPlayerAsync(shared_ptr<Instance> player,
    boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!player)
    {
        errorFunction("PolicyService expected Player, got nil");
        return;
    }
    resumeFunction(buildPolicyInfo());
}

void PolicyService::getPolicyInfoForServerRobloxOnlyAsync(
    boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction,
    boost::function<void(std::string)>)
{
    resumeFunction(buildPolicyInfo());
}

} // namespace RBX
