#include "v8datamodel/LocalizationService.h"

#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/LocalizationTable.h"
#include "network/Player.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>

namespace RBX {

const char* const sLocalizationService = "LocalizationService";

namespace {

std::string normalizeLocale(std::string locale)
{
    const std::string::size_type suffix = locale.find_first_of(".@");
    if (suffix != std::string::npos)
        locale.erase(suffix);
    std::replace(locale.begin(), locale.end(), '_', '-');
    std::transform(locale.begin(), locale.end(), locale.begin(),
        [](unsigned char value) { return static_cast<char>(std::tolower(value)); });
    if (locale.empty() || locale == "c" || locale == "posix")
        return "en-us";
    return locale;
}

std::string detectSystemLocale()
{
    const char* names[] = { "LC_ALL", "LC_MESSAGES", "LANG" };
    for (const char* name : names)
    {
        const char* value = std::getenv(name);
        if (value && *value)
            return normalizeLocale(value);
    }
    return "en-us";
}

} // namespace

static const Reflection::PropDescriptor<LocalizationService, std::string> prop_RobloxLocaleId(
    "RobloxLocaleId", category_Data, &LocalizationService::getRobloxLocaleId, NULL,
    Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
static const Reflection::PropDescriptor<LocalizationService, std::string> prop_SystemLocaleId(
    "SystemLocaleId", category_Data, &LocalizationService::getSystemLocaleId, NULL,
    Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
static const Reflection::PropDescriptor<LocalizationService, bool> prop_IsTextScraperRunning(
    "IsTextScraperRunning", category_Data, &LocalizationService::getIsTextScraperRunning,
    &LocalizationService::setIsTextScraperRunning, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING,
    Security::RobloxScript);

static const Reflection::BoundFuncDesc<LocalizationService, void(std::string)> func_SetRobloxLocaleId(
    &LocalizationService::setRobloxLocaleId, "SetRobloxLocaleId", "locale", Security::RobloxScript);
static const Reflection::BoundFuncDesc<LocalizationService, boost::shared_ptr<const Instances>()>
    func_GetCorescriptLocalizations(&LocalizationService::getCorescriptLocalizations,
        "GetCorescriptLocalizations", Security::None);
static const Reflection::BoundFuncDesc<LocalizationService,
    boost::shared_ptr<const Reflection::ValueArray>(boost::shared_ptr<Instance>)>
    func_GetTableEntries(&LocalizationService::getTableEntries, "GetTableEntries", "instance",
        boost::shared_ptr<Instance>(), Security::None);
static const Reflection::BoundFuncDesc<LocalizationService,
    boost::shared_ptr<Instance>(std::string)>
    func_GetTranslatorForLocaleAsync(&LocalizationService::getTranslatorForLocale,
        "GetTranslatorForLocaleAsync", "locale", Security::None);
static const Reflection::BoundFuncDesc<LocalizationService,
    boost::shared_ptr<Instance>(boost::shared_ptr<Instance>)>
    func_GetTranslatorForPlayer(&LocalizationService::getTranslatorForPlayer,
        "GetTranslatorForPlayer", "player", Security::None);
static const Reflection::BoundYieldFuncDesc<LocalizationService,
    std::string(boost::shared_ptr<Instance>)> func_GetCountryRegionForPlayerAsync(
        &LocalizationService::getCountryRegionForPlayerAsync,
        "GetCountryRegionForPlayerAsync", "player", Security::None);
static const Reflection::BoundFuncDesc<LocalizationService, bool()>
    func_GetIsLoadingInternalTranslations(&LocalizationService::getIsLoadingInternalTranslations,
        "GetIsLoadingInternalTranslations", Security::RobloxScript);
static const Reflection::BoundFuncDesc<LocalizationService, void(bool)>
    func_IsLoadingInternalTranslationsSettingChanged(
        &LocalizationService::setIsLoadingInternalTranslations,
        "IsLoadingInternalTranslationsSettingChanged", "newIsLoadingInternalTranslations",
        Security::RobloxScript);
static const Reflection::BoundFuncDesc<LocalizationService, void()> func_StartTextScraper(
    &LocalizationService::startTextScraper, "StartTextScraper", Security::RobloxScript);
static const Reflection::BoundFuncDesc<LocalizationService, void()> func_StopTextScraper(
    &LocalizationService::stopTextScraper, "StopTextScraper", Security::RobloxScript);
static const Reflection::EventDesc<LocalizationService, void()> event_AutoTranslateWillRun(
    &LocalizationService::autoTranslateWillRunSignal, "AutoTranslateWillRun", Security::RobloxScript);

LocalizationService::LocalizationService()
    : DescribedNonCreatable<LocalizationService, Instance, sLocalizationService,
          Reflection::ClassDescriptor::INTERNAL_LOCAL>(sLocalizationService)
    , Service(true)
    , systemLocaleId(detectSystemLocale())
    , robloxLocaleId(systemLocaleId)
    , loadingInternalTranslations(false)
    , textScraperRunning(false)
{
    setName(sLocalizationService);
    setRobloxLocked(true);
}

void LocalizationService::setRobloxLocaleId(std::string locale)
{
    locale = normalizeLocale(locale);
    if (robloxLocaleId != locale)
    {
        robloxLocaleId = locale;
        raisePropertyChanged(prop_RobloxLocaleId);
        autoTranslateWillRunSignal();
    }
}

boost::shared_ptr<LocalizationTable> LocalizationService::findCoreLocalizationTable() const
{
    CoreGuiService* coreGui = ServiceProvider::find<CoreGuiService>(this);
    if (!coreGui)
        return boost::shared_ptr<LocalizationTable>();

    for (std::size_t index = 0; index < coreGui->numChildren(); ++index)
    {
        LocalizationTable* table = Instance::fastDynamicCast<LocalizationTable>(coreGui->getChild(index));
        if (table && table->getName() == "CoreScriptLocalization")
            return boost::dynamic_pointer_cast<LocalizationTable>(shared_from(table));
    }
    return boost::shared_ptr<LocalizationTable>();
}

boost::shared_ptr<const Instances> LocalizationService::getCorescriptLocalizations()
{
    boost::shared_ptr<Instances> result(new Instances());
    CoreGuiService* coreGui = ServiceProvider::find<CoreGuiService>(this);
    if (!coreGui)
        return result;
    for (std::size_t index = 0; index < coreGui->numChildren(); ++index)
    {
        LocalizationTable* table = Instance::fastDynamicCast<LocalizationTable>(coreGui->getChild(index));
        if (table)
            result->push_back(shared_from(table));
    }
    return result;
}

boost::shared_ptr<const Reflection::ValueArray> LocalizationService::getTableEntries(
    boost::shared_ptr<Instance> instance)
{
    LocalizationTable* table = Instance::fastDynamicCast<LocalizationTable>(instance.get());
    if (!table)
    {
        boost::shared_ptr<LocalizationTable> core = findCoreLocalizationTable();
        table = core.get();
    }
    if (!table)
        return boost::shared_ptr<const Reflection::ValueArray>(new Reflection::ValueArray());
    return table->getEntries();
}

boost::shared_ptr<Instance> LocalizationService::getTranslatorForLocale(std::string locale)
{
    boost::shared_ptr<LocalizationTable> table = findCoreLocalizationTable();
    if (!table)
        throw std::runtime_error("CoreScriptLocalization is unavailable");
    return table->getTranslator(normalizeLocale(locale));
}

boost::shared_ptr<Instance> LocalizationService::getTranslatorForPlayer(boost::shared_ptr<Instance> player)
{
    if (!player)
        throw std::runtime_error("GetTranslatorForPlayer requires a Player");
    return getTranslatorForLocale(robloxLocaleId);
}

void LocalizationService::getCountryRegionForPlayerAsync(
    boost::shared_ptr<Instance> player,
    boost::function<void(std::string)> resumeFunction,
    boost::function<void(std::string)> errorFunction)
{
    if (!Instance::fastDynamicCast<Network::Player>(player.get()))
    {
        errorFunction("GetCountryRegionForPlayerAsync requires a Player");
        return;
    }

    // Offline sessions have no remote account-region service. The local
    // platform locale is the authoritative available region source; a future
    // connected session can provide the same result through player metadata
    // without changing this reflected API or its yielding behavior.
    const std::string::size_type separator = robloxLocaleId.rfind('-');
    std::string region = separator == std::string::npos
        ? std::string("US")
        : robloxLocaleId.substr(separator + 1);
    if (region.size() != 2)
        region = "US";
    std::transform(region.begin(), region.end(), region.begin(),
        [](unsigned char value) { return static_cast<char>(std::toupper(value)); });
    resumeFunction(region);
}

void LocalizationService::setIsLoadingInternalTranslations(bool value)
{
    loadingInternalTranslations = value;
}

void LocalizationService::setIsTextScraperRunning(bool value)
{
    if (textScraperRunning != value)
    {
        textScraperRunning = value;
        raisePropertyChanged(prop_IsTextScraperRunning);
    }
}

} // namespace RBX
