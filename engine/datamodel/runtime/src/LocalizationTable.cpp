#include "v8datamodel/LocalizationTable.h"
#include "v8xml/WebParser.h"

#include <rapidjson/document.h>
#include <boost/algorithm/string/replace.hpp>
#include <sstream>

namespace RBX {

const char* const sLocalizationTable = "LocalizationTable";
const char* const sTranslator = "Translator";

static const Reflection::PropDescriptor<LocalizationTable, std::string> prop_Contents(
    "Contents", category_Data, &LocalizationTable::getContents, &LocalizationTable::setContents);
static const Reflection::PropDescriptor<LocalizationTable, std::string> prop_SourceLocaleId(
    "SourceLocaleId", category_Data, &LocalizationTable::getSourceLocaleId,
    &LocalizationTable::setSourceLocaleId);
static const Reflection::PropDescriptor<Translator, std::string> prop_LocaleId(
    "LocaleId", category_Data, &Translator::getLocaleId, NULL,
    Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);

static const Reflection::BoundFuncDesc<LocalizationTable, boost::shared_ptr<Instance>(std::string)>
    func_GetTranslator(&LocalizationTable::getTranslator, "GetTranslator", "localeId", Security::None);
static const Reflection::BoundFuncDesc<LocalizationTable, boost::shared_ptr<const Reflection::ValueArray>()>
    func_GetEntries(&LocalizationTable::getEntries, "GetEntries", Security::None);
static const Reflection::BoundFuncDesc<LocalizationTable, void(Reflection::Variant)>
    func_SetEntries(&LocalizationTable::setEntries, "SetEntries", "entries", Security::None);
static const Reflection::BoundFuncDesc<Translator, std::string(std::string, Reflection::Variant)>
    func_FormatByKey(&Translator::formatByKey, "FormatByKey", "key", "arguments",
        Reflection::Variant(), Security::None);
static const Reflection::BoundFuncDesc<Translator, std::string(boost::shared_ptr<Instance>, std::string)>
    func_Translate(&Translator::translate, "Translate", "context", "text", Security::None);

namespace {

const rapidjson::Value* member(const rapidjson::Value& value, const char* lower, const char* upper)
{
    if (!value.IsObject())
        return NULL;
    if (value.HasMember(lower))
        return &value[lower];
    return value.HasMember(upper) ? &value[upper] : NULL;
}

std::string variantString(const Reflection::Variant& value)
{
    if (value.isType<std::string>())
        return value.get<std::string>();
    if (value.isType<double>())
    {
        std::ostringstream stream;
        stream << value.get<double>();
        return stream.str();
    }
    if (value.isType<int>())
        return RBX::format("%d", value.get<int>());
    if (value.isType<bool>())
        return value.get<bool>() ? "true" : "false";
    return std::string();
}

void replaceArguments(std::string& text, const Reflection::Variant& arguments)
{
    const Reflection::ValueTable* table = NULL;
    const Reflection::ValueMap* map = NULL;
    if (arguments.isType<boost::shared_ptr<const Reflection::ValueTable> >())
        table = arguments.get<boost::shared_ptr<const Reflection::ValueTable> >().get();
    else if (arguments.isType<boost::shared_ptr<const Reflection::ValueMap> >())
        map = arguments.get<boost::shared_ptr<const Reflection::ValueMap> >().get();

    if (table)
    {
        for (Reflection::ValueTable::const_iterator it = table->begin(); it != table->end(); ++it)
            boost::replace_all(text, "{" + it->first + "}", variantString(it->second));
    }
    if (map)
    {
        for (Reflection::ValueMap::const_iterator it = map->begin(); it != map->end(); ++it)
            boost::replace_all(text, "{" + it->first + "}", variantString(it->second));
    }
}

} // namespace

LocalizationTable::LocalizationTable()
    : DescribedCreatable<LocalizationTable, Instance, sLocalizationTable>(sLocalizationTable)
    , contents("[]")
    , sourceLocaleId("en-us")
{
}

void LocalizationTable::setContents(const std::string& value)
{
    rapidjson::Document document;
    document.Parse<rapidjson::kParseDefaultFlags>(value.c_str());
    if (document.HasParseError() || !document.IsArray())
        throw std::runtime_error("LocalizationTable.Contents must be a JSON array");
    if (contents != value)
    {
        rebuildCache(value);
        contents = value;
        raisePropertyChanged(prop_Contents);
    }
}

void LocalizationTable::rebuildCache(const std::string& value)
{
    rapidjson::Document document;
    document.Parse<rapidjson::kParseDefaultFlags>(value.c_str());
    if (document.HasParseError() || !document.IsArray())
        throw std::runtime_error("LocalizationTable.Contents must be a JSON array");

    std::map<std::string, CachedEntry> byKey;
    std::map<std::string, CachedEntry> bySource;
    for (rapidjson::Value::ConstValueIterator it = document.Begin(); it != document.End(); ++it)
    {
        const rapidjson::Value* source = member(*it, "source", "Source");
        const rapidjson::Value* key = member(*it, "key", "Key");
        if ((!source || !source->IsString()) && (!key || !key->IsString()))
            continue;

        CachedEntry entry;
        if (source && source->IsString())
            entry.source = source->GetString();
        const rapidjson::Value* values = member(*it, "values", "Values");
        if (values && values->IsObject())
            for (rapidjson::Value::ConstMemberIterator valueIt = values->MemberBegin();
                 valueIt != values->MemberEnd(); ++valueIt)
                if (valueIt->name.IsString() && valueIt->value.IsString())
                    entry.values[valueIt->name.GetString()] = valueIt->value.GetString();

        // Match the serialized-table behavior: the first matching entry wins.
        if (key && key->IsString() && !byKey.count(key->GetString()))
            byKey[key->GetString()] = entry;
        if (!entry.source.empty() && !bySource.count(entry.source))
            bySource[entry.source] = entry;
    }
    entriesByKey.swap(byKey);
    entriesBySource.swap(bySource);
}

void LocalizationTable::setSourceLocaleId(const std::string& value)
{
    if (sourceLocaleId != value)
    {
        sourceLocaleId = value;
        raisePropertyChanged(prop_SourceLocaleId);
    }
}

boost::shared_ptr<Instance> LocalizationTable::getTranslator(std::string localeId)
{
    return boost::shared_ptr<Translator>(new Translator(
        boost::dynamic_pointer_cast<LocalizationTable>(shared_from(this)), localeId));
}

boost::shared_ptr<const Reflection::ValueArray> LocalizationTable::getEntries()
{
    boost::shared_ptr<const Reflection::ValueArray> entries;
    if (!WebParser::parseJSONArray(contents, entries))
        throw std::runtime_error("LocalizationTable contains invalid JSON");
    return entries;
}

void LocalizationTable::setEntries(Reflection::Variant entries)
{
    std::string json;
    if (!WebParser::writeJSON(entries, json, WebParser::FailOnNonJSON))
        throw std::runtime_error("LocalizationTable:SetEntries expects an array of dictionaries");
    setContents(json);
}

std::string LocalizationTable::format(const std::string& localeId, const std::string& key,
    const Reflection::Variant& arguments) const
{
    std::map<std::string, CachedEntry>::const_iterator found = entriesByKey.find(key);
    if (found == entriesByKey.end())
        throw std::runtime_error("Localization key not found: " + key);
    std::string text = found->second.source;
    std::map<std::string, std::string>::const_iterator translation =
        found->second.values.find(localeId);
    if (translation != found->second.values.end())
        text = translation->second;
    if (text.empty())
        throw std::runtime_error("Localization key has no source or target text: " + key);
    replaceArguments(text, arguments);
    return text;
}

std::string LocalizationTable::translate(const std::string& localeId, const std::string& source) const
{
    std::map<std::string, CachedEntry>::const_iterator found = entriesBySource.find(source);
    if (found != entriesBySource.end())
    {
        std::map<std::string, std::string>::const_iterator translation =
            found->second.values.find(localeId);
        if (translation != found->second.values.end())
            return translation->second;
    }
    return source;
}

Translator::Translator(const boost::shared_ptr<LocalizationTable>& table, const std::string& localeId)
    : DescribedNonCreatable<Translator, Instance, sTranslator>(sTranslator)
    , table(table)
    , localeId(localeId)
{
}

std::string Translator::formatByKey(std::string key, Reflection::Variant arguments)
{
    boost::shared_ptr<LocalizationTable> owner = table.lock();
    if (!owner)
        throw std::runtime_error("Translator's LocalizationTable no longer exists");
    return owner->format(localeId, key, arguments);
}

std::string Translator::translate(boost::shared_ptr<Instance>, std::string source)
{
    boost::shared_ptr<LocalizationTable> owner = table.lock();
    if (!owner)
        throw std::runtime_error("Translator's LocalizationTable no longer exists");
    return owner->translate(localeId, source);
}

} // namespace RBX
