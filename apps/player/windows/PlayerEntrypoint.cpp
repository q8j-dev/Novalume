#include "PlayerMain.h"

#include <Windows.h>
#include <shellapi.h>

#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace {

std::string toUtf8(const wchar_t* value)
{
    if (!value)
        return {};

    const int required = WideCharToMultiByte(
        CP_UTF8, WC_ERR_INVALID_CHARS, value, -1, nullptr, 0, nullptr, nullptr);
    if (required <= 0)
        throw std::runtime_error("Windows command line contains invalid UTF-16");

    std::string result(static_cast<std::size_t>(required), '\0');
    if (WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
            value, -1, result.data(), required, nullptr, nullptr) <= 0)
        throw std::runtime_error("Windows command line could not be converted to UTF-8");
    result.pop_back();
    return result;
}

std::wstring toWide(std::string_view value)
{
    const int required = value.empty() ? 0 : MultiByteToWideChar(
        CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
        static_cast<int>(value.size()), nullptr, 0);
    if (required <= 0)
        return {};
    std::wstring result(static_cast<std::size_t>(required), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
            static_cast<int>(value.size()), result.data(), required) <= 0)
        return {};
    return result;
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    try {
        int wideArgumentCount = 0;
        LPWSTR* wideArguments = CommandLineToArgvW(
            GetCommandLineW(), &wideArgumentCount);
        if (!wideArguments)
            throw std::runtime_error("Windows could not parse the command line");

        struct LocalFreeGuard final {
            LPWSTR* value;
            ~LocalFreeGuard() { LocalFree(value); }
        } guard{wideArguments};

        std::vector<std::string> arguments;
        arguments.reserve(static_cast<std::size_t>(wideArgumentCount));
        for (int index = 0; index < wideArgumentCount; ++index)
            arguments.push_back(toUtf8(wideArguments[index]));

        std::vector<char*> argumentPointers;
        argumentPointers.reserve(arguments.size());
        for (std::string& argument : arguments)
            argumentPointers.push_back(argument.data());

        return rbxPlayerMain(
            static_cast<int>(argumentPointers.size()), argumentPointers.data());
    } catch (const std::exception& error) {
        const std::wstring message = toWide(error.what());
        if (!message.empty())
            MessageBoxW(nullptr, message.c_str(), L"Roblox Player", MB_OK | MB_ICONERROR);
        else
            MessageBoxA(nullptr, error.what(), "Roblox Player", MB_OK | MB_ICONERROR);
        return 1;
    }
}
