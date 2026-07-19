#include "V8DataModel/ThumbnailEnums.h"

#include "reflection/enumconverter.h"

namespace RBX {
namespace Reflection {

template<> EnumDesc<ThumbnailType>::EnumDesc() : EnumDescriptor("ThumbnailType")
{
    addPair(THUMBNAIL_HEAD_SHOT, "HeadShot");
    addPair(THUMBNAIL_AVATAR_BUST, "AvatarBust");
    addPair(THUMBNAIL_AVATAR, "AvatarThumbnail");
}

template<> EnumDesc<ThumbnailSize>::EnumDesc() : EnumDescriptor("ThumbnailSize")
{
    addPair(THUMBNAIL_SIZE_48, "Size48x48");
    addPair(THUMBNAIL_SIZE_180, "Size180x180");
    addPair(THUMBNAIL_SIZE_420, "Size420x420");
    addPair(THUMBNAIL_SIZE_60, "Size60x60");
    addPair(THUMBNAIL_SIZE_100, "Size100x100");
    addPair(THUMBNAIL_SIZE_150, "Size150x150");
    addPair(THUMBNAIL_SIZE_352, "Size352x352");
}

template<> ThumbnailType& Variant::convert<ThumbnailType>()
{ return genericConvert<ThumbnailType>(); }
template<> ThumbnailSize& Variant::convert<ThumbnailSize>()
{ return genericConvert<ThumbnailSize>(); }

} // namespace Reflection

template<> bool StringConverter<ThumbnailType>::convertToValue(
    const std::string& text, ThumbnailType& value)
{
    return Reflection::EnumDesc<ThumbnailType>::singleton().convertToValue(
        text.c_str(), value);
}
template<> bool StringConverter<ThumbnailSize>::convertToValue(
    const std::string& text, ThumbnailSize& value)
{
    return Reflection::EnumDesc<ThumbnailSize>::singleton().convertToValue(
        text.c_str(), value);
}

} // namespace RBX
