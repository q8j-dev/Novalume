#include "rbx/platform/Clipboard.h"

#include <SDL3/SDL.h>

#include <string>

namespace rbx::platform {

std::string readClipboardText()
{
    char* value = SDL_GetClipboardText();
    if (!value)
        return {};
    std::string result(value);
    SDL_free(value);
    return result;
}

} // namespace rbx::platform
