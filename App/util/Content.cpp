#include "Util/Content.h"

#include "Reflection/EnumConverter.h"
#include "Reflection/Type.h"

#include <stdexcept>
#include <tuple>

namespace RBX {

Content Content::fromUri(std::string value)
{
    Content result;
    if (!value.empty())
    {
        result.sourceType = CONTENT_SOURCE_URI;
        result.uri = std::move(value);
    }
    return result;
}

Content Content::fromAssetId(std::int64_t assetId)
{
    if (assetId < 0)
        throw std::invalid_argument("assetId must not be negative");
    return assetId == 0 ? Content() : fromUri("rbxassetid://" + std::to_string(assetId));
}

Content Content::fromObject(boost::shared_ptr<Reflection::DescribedBase> value)
{
    if (!value)
        throw std::invalid_argument("object must not be nil");
    Content result;
    result.sourceType = CONTENT_SOURCE_OBJECT;
    result.object = std::move(value);
    return result;
}

Content Content::fromOpaque(boost::shared_ptr<const OpaqueContent> value)
{
    if (!value)
        throw std::invalid_argument("opaque content must not be nil");
    Content result;
    result.sourceType = CONTENT_SOURCE_OPAQUE;
    result.opaque = std::move(value);
    return result;
}

bool operator==(const Content& left, const Content& right)
{
    return left.sourceType == right.sourceType && left.uri == right.uri &&
        left.object == right.object && left.opaque == right.opaque;
}

bool operator<(const Content& left, const Content& right)
{
    return std::tie(left.sourceType, left.uri, left.object, left.opaque) <
        std::tie(right.sourceType, right.uri, right.object, right.opaque);
}

namespace Reflection {
template<> EnumDesc<RBX::ContentSourceType>::EnumDesc()
    : EnumDescriptor("ContentSourceType")
{
    addPair(RBX::CONTENT_SOURCE_NONE, "None");
    addPair(RBX::CONTENT_SOURCE_URI, "Uri");
    addPair(RBX::CONTENT_SOURCE_OBJECT, "Object");
    addPair(RBX::CONTENT_SOURCE_OPAQUE, "Opaque");
}

template<> const Type& Type::getSingleton<RBX::Content>()
{
    static TType<RBX::Content> type("Content");
    return type;
}
} // namespace Reflection

} // namespace RBX
