#pragma once

namespace RBX {
namespace Enums {

enum HttpCachePolicy
{
    HTTP_CACHE_POLICY_NONE = 0,
    HTTP_CACHE_POLICY_FULL = 1,
    HTTP_CACHE_POLICY_DATA_ONLY = 2,
    HTTP_CACHE_POLICY_DEFAULT = 3,
    HTTP_CACHE_POLICY_INTERNAL_REDIRECT_REFRESH = 4
};

} // namespace Enums
} // namespace RBX
