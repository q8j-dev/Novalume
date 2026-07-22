#include "rbx/platform/Clipboard.h"

#include <emscripten.h>
#include <emscripten/em_asm.h>

#include <string>

namespace rbx::platform {

char* readWebClipboard()
{
    return reinterpret_cast<char*>(MAIN_THREAD_EM_ASM_PTR({
        const value = globalThis.__novalumeClipboard || String();
        const size = lengthBytesUTF8(value) + 1;
        const result = _malloc(size);
        stringToUTF8(value, result, size);
        return result;
    }));
}

std::string readClipboardText()
{
    char* value = readWebClipboard();
    std::string result = value ? value : "";
    std::free(value);
    return result;
}

}
