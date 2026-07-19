#include "GfxBase/ThumbnailTextureProvider.h"

#include <charconv>
#include <limits>
#include <map>
#include <string_view>

namespace RBX {
namespace {

bool parsePositiveInteger(std::string_view text, std::int64_t maximum,
    std::int64_t& value)
{
    if (text.empty())
        return false;

    std::int64_t parsed = 0;
    const std::from_chars_result result =
        std::from_chars(text.data(), text.data() + text.size(), parsed);
    if (result.ec != std::errc() || result.ptr != text.data() + text.size() ||
        parsed <= 0 || parsed > maximum)
        return false;

    value = parsed;
    return true;
}

} // namespace

bool parseThumbnailSceneRequest(std::string_view value,
    ThumbnailSceneRequest& request)
{
    constexpr std::string_view prefix = "rbxthumb://";
    if (!value.starts_with(prefix))
        return false;

    std::map<std::string_view, std::string_view> parameters;
    std::string_view query(value.data() + prefix.size(), value.size() - prefix.size());
    while (!query.empty())
    {
        const std::size_t separator = query.find('&');
        const std::string_view field = query.substr(0, separator);
        const std::size_t equals = field.find('=');
        if (equals == std::string_view::npos || equals == 0 ||
            equals + 1 == field.size())
            return false;
        if (!parameters.emplace(field.substr(0, equals),
                field.substr(equals + 1)).second)
            return false;
        if (separator == std::string_view::npos)
            break;
        query.remove_prefix(separator + 1);
    }

    const auto type = parameters.find("type");
    const auto id = parameters.find("id");
    const auto width = parameters.find("w");
    const auto height = parameters.find("h");
    if (type == parameters.end() || id == parameters.end() ||
        width == parameters.end() || height == parameters.end())
        return false;

    ThumbnailSceneType parsedType;
    if (type->second == "Avatar" || type->second == "AvatarThumbnail")
        parsedType = ThumbnailSceneType::Avatar;
    else if (type->second == "AvatarBust")
        parsedType = ThumbnailSceneType::AvatarBust;
    else if (type->second == "AvatarHeadShot" || type->second == "HeadShot")
        parsedType = ThumbnailSceneType::AvatarHeadShot;
    else
        return false;

    std::int64_t parsedId = 0;
    std::int64_t parsedWidth = 0;
    std::int64_t parsedHeight = 0;
    if (!parsePositiveInteger(id->second, std::numeric_limits<std::int32_t>::max(),
            parsedId) ||
        !parsePositiveInteger(width->second, 2048, parsedWidth) ||
        !parsePositiveInteger(height->second, 2048, parsedHeight))
        return false;

    request.type = parsedType;
    request.userId = parsedId;
    request.size = Vector2(static_cast<float>(parsedWidth),
        static_cast<float>(parsedHeight));
    request.cacheKey.assign(value);
    return true;
}

bool parseThumbnailSceneRequest(const ContentId& content,
    ThumbnailSceneRequest& request)
{
    return parseThumbnailSceneRequest(content.toString(), request);
}

} // namespace RBX
