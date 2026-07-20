#include "rbx/platform/Clipboard.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <stdexcept>
#include <cstring>
#include <string>
#include <string_view>

namespace rbx::platform {
namespace {

std::wstring widen(std::string_view value)
{
    if (value.empty())
        return {};
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
        value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (size <= 0)
        throw std::runtime_error("clipboard text is not valid UTF-8");
    std::wstring result(static_cast<std::size_t>(size), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), size) != size)
        throw std::runtime_error("failed to convert clipboard text to UTF-16");
    return result;
}

std::string narrow(std::wstring_view value)
{
    if (value.empty())
        return {};
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
        value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    if (size <= 0)
        throw std::runtime_error("clipboard text is not valid UTF-16");
    std::string result(static_cast<std::size_t>(size), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), size, nullptr, nullptr) != size)
        throw std::runtime_error("failed to convert clipboard text to UTF-8");
    return result;
}

} // namespace

std::string readClipboardText()
{
    if (!OpenClipboard(nullptr))
        return {};
    const HANDLE data = GetClipboardData(CF_UNICODETEXT);
    if (!data) {
        CloseClipboard();
        return {};
    }
    const auto* text = static_cast<const wchar_t*>(GlobalLock(data));
    std::string result;
    if (text) {
        result = narrow(text);
        GlobalUnlock(data);
    }
    CloseClipboard();
    return result;
}

void writeClipboardText(std::string_view text)
{
    const std::wstring wide = widen(text);
    const std::size_t bytes = (wide.size() + 1) * sizeof(wchar_t);
    HGLOBAL memory = GlobalAlloc(GMEM_MOVEABLE, bytes);
    if (!memory)
        throw std::runtime_error("failed to allocate clipboard storage");
    void* destination = GlobalLock(memory);
    if (!destination) {
        GlobalFree(memory);
        throw std::runtime_error("failed to lock clipboard storage");
    }
    std::memcpy(destination, wide.c_str(), bytes);
    GlobalUnlock(memory);
    if (!OpenClipboard(nullptr)) {
        GlobalFree(memory);
        throw std::runtime_error("failed to open the Windows clipboard");
    }
    if (!EmptyClipboard() || !SetClipboardData(CF_UNICODETEXT, memory)) {
        CloseClipboard();
        GlobalFree(memory);
        throw std::runtime_error("failed to publish Windows clipboard text");
    }
    CloseClipboard();
}

} // namespace rbx::platform
