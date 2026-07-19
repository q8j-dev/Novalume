#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

namespace RBX {

extern const char* const sLocalizationService;

class LocalizationService
    : public DescribedNonCreatable<LocalizationService, Instance, sLocalizationService,
          Reflection::ClassDescriptor::INTERNAL_LOCAL>
    , public Service
{
public:
    LocalizationService();

    const std::string& getRobloxLocaleId() const { return robloxLocaleId; }
    const std::string& getSystemLocaleId() const { return systemLocaleId; }
    void setRobloxLocaleId(std::string locale);

    boost::shared_ptr<const Instances> getCorescriptLocalizations();
    boost::shared_ptr<const Reflection::ValueArray> getTableEntries(
        boost::shared_ptr<Instance> instance);
    boost::shared_ptr<Instance> getTranslatorForLocale(std::string locale);
    boost::shared_ptr<Instance> getTranslatorForPlayer(boost::shared_ptr<Instance> player);
    void getCountryRegionForPlayerAsync(boost::shared_ptr<Instance> player,
        boost::function<void(std::string)> resumeFunction,
        boost::function<void(std::string)> errorFunction);

    bool getIsLoadingInternalTranslations() { return loadingInternalTranslations; }
    void setIsLoadingInternalTranslations(bool value);
    bool getIsTextScraperRunning() const { return textScraperRunning; }
    void setIsTextScraperRunning(bool value);
    void startTextScraper() { setIsTextScraperRunning(true); }
    void stopTextScraper() { setIsTextScraperRunning(false); }

    rbx::signal<void()> autoTranslateWillRunSignal;

    boost::shared_ptr<class LocalizationTable> findCoreLocalizationTable() const;

private:
    std::string systemLocaleId;
    std::string robloxLocaleId;
    bool loadingInternalTranslations;
    bool textScraperRunning;
};

} // namespace RBX
