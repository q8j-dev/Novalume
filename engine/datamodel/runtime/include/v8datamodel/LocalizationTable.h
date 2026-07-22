#pragma once

#include "v8tree/Instance.h"
#include "reflection/Type.h"

#include <map>

namespace RBX {

extern const char* const sLocalizationTable;
extern const char* const sTranslator;

class LocalizationTable;

class Translator : public DescribedNonCreatable<Translator, Instance, sTranslator>
{
public:
    Translator(const boost::shared_ptr<LocalizationTable>& table, const std::string& localeId);

    const std::string& getLocaleId() const { return localeId; }
    std::string formatByKey(std::string key, Reflection::Variant arguments);
    std::string translate(boost::shared_ptr<Instance> context, std::string source);

private:
    boost::weak_ptr<LocalizationTable> table;
    std::string localeId;
};

class LocalizationTable
    : public DescribedCreatable<LocalizationTable, Instance, sLocalizationTable>
{
public:
    LocalizationTable();

    const std::string& getContents() const { return contents; }
    void setContents(const std::string& value);
    const std::string& getSourceLocaleId() const { return sourceLocaleId; }
    void setSourceLocaleId(const std::string& value);

    boost::shared_ptr<Instance> getTranslator(std::string localeId);
    boost::shared_ptr<const Reflection::ValueArray> getEntries();
    void setEntries(Reflection::Variant entries);

    std::string format(const std::string& localeId, const std::string& key,
        const Reflection::Variant& arguments) const;
    std::string translate(const std::string& localeId, const std::string& source) const;

private:
    struct CachedEntry
    {
        std::string source;
        std::map<std::string, std::string> values;
    };

    void rebuildCache(const std::string& value);

    std::string contents;
    std::string sourceLocaleId;
    std::map<std::string, CachedEntry> entriesByKey;
    std::map<std::string, CachedEntry> entriesBySource;
};

} // namespace RBX
