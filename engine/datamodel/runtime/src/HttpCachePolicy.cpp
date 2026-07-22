#include "v8datamodel/HttpCachePolicy.h"

#include "reflection/EnumConverter.h"

namespace RBX {
namespace Reflection {

template<> EnumDesc<Enums::HttpCachePolicy>::EnumDesc()
    : EnumDescriptor("HttpCachePolicy")
{
    addPair(Enums::HTTP_CACHE_POLICY_NONE, "None");
    addPair(Enums::HTTP_CACHE_POLICY_FULL, "Full");
    addPair(Enums::HTTP_CACHE_POLICY_DATA_ONLY, "DataOnly");
    addPair(Enums::HTTP_CACHE_POLICY_DEFAULT, "Default");
    addPair(Enums::HTTP_CACHE_POLICY_INTERNAL_REDIRECT_REFRESH, "InternalRedirectRefresh");
}

template<> Enums::HttpCachePolicy& Variant::convert<Enums::HttpCachePolicy>()
{
    return genericConvert<Enums::HttpCachePolicy>();
}

} // namespace Reflection

template<> bool StringConverter<Enums::HttpCachePolicy>::convertToValue(
    const std::string& text, Enums::HttpCachePolicy& value)
{
    return Reflection::EnumDesc<Enums::HttpCachePolicy>::singleton()
        .convertToValue(text.c_str(), value);
}

} // namespace RBX
