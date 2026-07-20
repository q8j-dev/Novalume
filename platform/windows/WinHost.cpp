#include "rbx/platform/Host.h"
#include "rbx/platform/RecentDocuments.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <xinput.h>
#include <knownfolders.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shobjidl.h>

#include <array>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace rbx::platform {
namespace {

std::filesystem::path executablePath()
{
    std::wstring buffer(32768, L'\0');
    const DWORD length = GetModuleFileNameW(nullptr, buffer.data(),
        static_cast<DWORD>(buffer.size()));
    if (length == 0 || length == buffer.size())
        throw std::runtime_error("failed to resolve the Player executable path");
    buffer.resize(length);
    return std::filesystem::path(buffer);
}

std::filesystem::path knownFolder(REFKNOWNFOLDERID identifier)
{
    PWSTR value = nullptr;
    if (FAILED(SHGetKnownFolderPath(identifier, KF_FLAG_DEFAULT, nullptr, &value)))
        return {};
    std::filesystem::path result(value);
    CoTaskMemFree(value);
    return result;
}

std::wstring quoteArgument(std::wstring_view value)
{
    std::wstring result = L"\"";
    std::size_t slashes = 0;
    for (const wchar_t character : value) {
        if (character == L'\\') {
            ++slashes;
            continue;
        }
        if (character == L'\"') {
            result.append(slashes * 2 + 1, L'\\');
            result.push_back(character);
        } else {
            result.append(slashes, L'\\');
            result.push_back(character);
        }
        slashes = 0;
    }
    result.append(slashes * 2, L'\\');
    result.push_back(L'\"');
    return result;
}

std::uint32_t modifiers()
{
    std::uint32_t result = 0;
    result |= (GetKeyState(VK_SHIFT) < 0) ? 1U << 0U : 0U;
    result |= (GetKeyState(VK_CONTROL) < 0) ? 1U << 1U : 0U;
    result |= (GetKeyState(VK_MENU) < 0) ? 1U << 2U : 0U;
    result |= ((GetKeyState(VK_LWIN) < 0) || (GetKeyState(VK_RWIN) < 0))
        ? 1U << 3U : 0U;
    result |= (GetKeyState(VK_CAPITAL) & 1) ? 1U << 4U : 0U;
    return result;
}

InputEvent::Key translateKey(WPARAM key, LPARAM flags)
{
    using Key = InputEvent::Key;
    if (key >= '0' && key <= '9')
        return static_cast<Key>(static_cast<unsigned>(Key::zero) + key - '0');
    if (key >= 'A' && key <= 'Z')
        return static_cast<Key>(static_cast<unsigned>(Key::a) + key - 'A');
    if (key >= VK_F1 && key <= VK_F12)
        return static_cast<Key>(static_cast<unsigned>(Key::f1) + key - VK_F1);
    switch (key) {
    case VK_BACK: return Key::backspace;
    case VK_TAB: return Key::tab;
    case VK_RETURN: return Key::enter;
    case VK_ESCAPE: return Key::escape;
    case VK_SPACE: return Key::space;
    case VK_OEM_7: return Key::quote;
    case VK_OEM_COMMA: return Key::comma;
    case VK_OEM_MINUS: return Key::minus;
    case VK_OEM_PERIOD: return Key::period;
    case VK_OEM_2: return Key::slash;
    case VK_OEM_1: return Key::semicolon;
    case VK_OEM_PLUS: return Key::equals;
    case VK_OEM_4: return Key::leftBracket;
    case VK_OEM_5: return Key::backslash;
    case VK_OEM_6: return Key::rightBracket;
    case VK_OEM_3: return Key::backquote;
    case VK_LEFT: return Key::left;
    case VK_RIGHT: return Key::right;
    case VK_UP: return Key::up;
    case VK_DOWN: return Key::down;
    case VK_SHIFT:
        return MapVirtualKeyW((flags >> 16) & 0xffU, MAPVK_VSC_TO_VK_EX) == VK_RSHIFT
            ? Key::rightShift : Key::leftShift;
    case VK_CONTROL: return (flags & (1LL << 24)) ? Key::rightControl : Key::leftControl;
    case VK_MENU: return (flags & (1LL << 24)) ? Key::rightAlt : Key::leftAlt;
    case VK_LWIN: return Key::leftMeta;
    case VK_RWIN: return Key::rightMeta;
    default: return Key::unknown;
    }
}

char translatedText(WPARAM key, LPARAM flags)
{
    std::array<BYTE, 256> keyboard{};
    if (!GetKeyboardState(keyboard.data()))
        return 0;
    wchar_t text[4]{};
    const int length = ToUnicode(static_cast<UINT>(key),
        static_cast<UINT>((flags >> 16) & 0xffU), keyboard.data(), text, 4, 0);
    return length == 1 && text[0] > 0 && text[0] < 128
        ? static_cast<char>(text[0]) : 0;
}

class WinHost final : public Host {
public:
    WinHost(std::uint32_t width, std::uint32_t height, bool visible)
        : visible_(visible)
    {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        registerWindowClass();
        const DWORD style = WS_OVERLAPPEDWINDOW;
        RECT rectangle{0, 0, static_cast<LONG>(width), static_cast<LONG>(height)};
        AdjustWindowRectExForDpi(&rectangle, style, FALSE, 0, USER_DEFAULT_SCREEN_DPI);
        window_ = CreateWindowExW(0, windowClassName(), L"Roblox", style,
            CW_USEDEFAULT, CW_USEDEFAULT, rectangle.right - rectangle.left,
            rectangle.bottom - rectangle.top, nullptr, nullptr,
            GetModuleHandleW(nullptr), this);
        if (!window_)
            throw std::runtime_error("failed to create the Windows Player window");
        DragAcceptFiles(window_, TRUE);
        if (visible_) {
            ShowWindow(window_, SW_SHOW);
            UpdateWindow(window_);
            SetForegroundWindow(window_);
        }
    }

    ~WinHost() override
    {
        pointerLockRequested_ = false;
        updatePointerLock();
        if (window_)
            DestroyWindow(window_);
    }

    NativeSurface nativeSurface() const noexcept override
    {
        RECT rectangle{};
        GetClientRect(window_, &rectangle);
        const std::uint32_t width = static_cast<std::uint32_t>(rectangle.right);
        const std::uint32_t height = static_cast<std::uint32_t>(rectangle.bottom);
        const float density = static_cast<float>(GetDpiForWindow(window_)) /
                              static_cast<float>(USER_DEFAULT_SCREEN_DPI);
        return NativeSurface{
            .window = reinterpret_cast<std::uintptr_t>(window_),
            .width = width,
            .height = height,
            .logicalWidth = static_cast<std::uint32_t>(width / density),
            .logicalHeight = static_cast<std::uint32_t>(height / density),
            .pixelDensity = density};
    }

    std::filesystem::path resourceRoot() const override
    {
        return executablePath().parent_path() / "Resources";
    }

    std::filesystem::path writableDataRoot() const override
    {
        auto path = knownFolder(FOLDERID_LocalAppData) / "Novalume";
        std::error_code error;
        std::filesystem::create_directories(path, error);
        return path;
    }

    std::filesystem::path existingClientSettingsRoot() const override
    {
        return knownFolder(FOLDERID_LocalAppData) / "Roblox" / "ClientSettings";
    }

    bool pumpEvents() override
    {
        MSG message{};
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT)
                running_ = false;
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }
        pollGamepads();
        updatePointerLock();
        return running_ && (!visible_ || IsWindow(window_));
    }

    std::vector<InputEvent> takeInputEvents() override
    {
        std::vector<InputEvent> result;
        result.swap(events_);
        return result;
    }

    void requestOpenDocument() override
    {
        const bool restoreLock = pointerLockRequested_;
        setPointerLock(false);
        const HRESULT initialized = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        IFileOpenDialog* dialog = nullptr;
        if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, nullptr,
                CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
            const COMDLG_FILTERSPEC filters[] = {
                {L"Roblox places and models", L"*.rbxl;*.rbxlx;*.rbxm;*.rbxmx;*.rbxlp"},
                {L"All files", L"*.*"}};
            dialog->SetFileTypes(static_cast<UINT>(std::size(filters)), filters);
            dialog->SetOptions(FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST |
                               FOS_FORCEFILESYSTEM | FOS_NOCHANGEDIR);
            if (SUCCEEDED(dialog->Show(window_))) {
                IShellItem* item = nullptr;
                if (SUCCEEDED(dialog->GetResult(&item))) {
                    PWSTR path = nullptr;
                    if (SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH, &path))) {
                        openedDocuments_.emplace_back(path);
                        CoTaskMemFree(path);
                    }
                    item->Release();
                }
            }
            dialog->Release();
        }
        if (SUCCEEDED(initialized))
            CoUninitialize();
        if (restoreLock)
            setPointerLock(true);
    }

    std::vector<std::filesystem::path> takeOpenedDocuments() override
    {
        std::vector<std::filesystem::path> result;
        result.swap(openedDocuments_);
        return result;
    }

    std::vector<std::filesystem::path> recentDocuments() const override
    {
        return loadRecentDocuments(writableDataRoot());
    }

    bool launchDocument(const std::filesystem::path& path) override
    {
        if (!std::filesystem::is_regular_file(path))
            return false;
        const std::wstring executable = executablePath().wstring();
        std::wstring command = quoteArgument(executable) + L" --place " +
                               quoteArgument(path.wstring());
        STARTUPINFOW startup{.cb = sizeof(startup)};
        PROCESS_INFORMATION process{};
        const BOOL launched = CreateProcessW(executable.c_str(), command.data(),
            nullptr, nullptr, FALSE, 0, nullptr, executablePath().parent_path().c_str(),
            &startup, &process);
        if (launched) {
            CloseHandle(process.hThread);
            CloseHandle(process.hProcess);
            recordRecentDocument(writableDataRoot(), path);
        }
        return launched != FALSE;
    }

    void setClipboardText(std::string_view text) override
    {
        const int size = text.empty() ? 0 : MultiByteToWideChar(CP_UTF8,
            MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (!text.empty() && size <= 0)
            return;
        std::wstring wide(static_cast<std::size_t>(size), L'\0');
        if (size > 0)
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                static_cast<int>(text.size()), wide.data(), size);
        HGLOBAL storage = GlobalAlloc(GMEM_MOVEABLE,
            (wide.size() + 1) * sizeof(wchar_t));
        if (!storage)
            return;
        void* destination = GlobalLock(storage);
        if (!destination) {
            GlobalFree(storage);
            return;
        }
        std::memcpy(destination, wide.c_str(), (wide.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(storage);
        if (!OpenClipboard(window_)) {
            GlobalFree(storage);
            return;
        }
        EmptyClipboard();
        if (!SetClipboardData(CF_UNICODETEXT, storage))
            GlobalFree(storage);
        CloseClipboard();
    }

    void setPointerLock(bool locked) override
    {
        pointerLockRequested_ = locked;
        updatePointerLock();
    }

private:
    static const wchar_t* windowClassName() { return L"NovalumePlayerWindow"; }

    static void registerWindowClass()
    {
        static std::once_flag once;
        std::call_once(once, [] {
            WNDCLASSEXW descriptor{
                .cbSize = sizeof(WNDCLASSEXW),
                .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
                .lpfnWndProc = windowProcedure,
                .hInstance = GetModuleHandleW(nullptr),
                .hIcon = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1)),
                .hCursor = LoadCursorW(nullptr, IDC_ARROW),
                .lpszClassName = windowClassName(),
                .hIconSm = LoadIconW(GetModuleHandleW(nullptr), MAKEINTRESOURCEW(1))};
            if (!RegisterClassExW(&descriptor) &&
                GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                throw std::runtime_error("failed to register the Windows Player window class");
        });
    }

    static LRESULT CALLBACK windowProcedure(HWND window, UINT message,
        WPARAM wParam, LPARAM lParam)
    {
        WinHost* host = reinterpret_cast<WinHost*>(
            GetWindowLongPtrW(window, GWLP_USERDATA));
        if (message == WM_NCCREATE) {
            const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lParam);
            host = static_cast<WinHost*>(create->lpCreateParams);
            host->window_ = window;
            SetWindowLongPtrW(window, GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(host));
        }
        return host ? host->handleMessage(message, wParam, lParam)
                    : DefWindowProcW(window, message, wParam, lParam);
    }

    LRESULT handleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message) {
        case WM_CLOSE:
            events_.push_back(InputEvent{.kind = InputEvent::Kind::nativeCloseRequested});
            running_ = false;
            return 0;
        case WM_DESTROY:
            running_ = false;
            return 0;
        case WM_SETFOCUS:
            focused_ = true;
            events_.push_back(InputEvent{.kind = InputEvent::Kind::focusGained});
            updatePointerLock();
            return 0;
        case WM_KILLFOCUS:
            focused_ = false;
            events_.push_back(InputEvent{.kind = InputEvent::Kind::focusLost});
            updatePointerLock();
            return 0;
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYUP: {
            const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
            if (down && wParam == 'O' && (GetKeyState(VK_CONTROL) < 0) &&
                (lParam & (1LL << 30)) == 0) {
                requestOpenDocument();
                return 0;
            }
            events_.push_back(InputEvent{
                .kind = down ? InputEvent::Kind::keyDown : InputEvent::Kind::keyUp,
                .key = translateKey(wParam, lParam),
                .modifiers = modifiers(),
                .text = down ? translatedText(wParam, lParam) : 0,
                .repeat = down && (lParam & (1LL << 30)) != 0});
            return 0;
        }
        case WM_MOUSEMOVE:
            if (!pointerLocked_)
                enqueuePointer(InputEvent::Kind::pointerMove,
                    InputEvent::PointerButton::none, GET_X_LPARAM(lParam),
                    GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MBUTTONUP: {
            const bool down = message == WM_LBUTTONDOWN || message == WM_RBUTTONDOWN ||
                              message == WM_MBUTTONDOWN;
            const auto button = (message == WM_LBUTTONDOWN || message == WM_LBUTTONUP)
                ? InputEvent::PointerButton::primary
                : (message == WM_RBUTTONDOWN || message == WM_RBUTTONUP)
                    ? InputEvent::PointerButton::secondary
                    : InputEvent::PointerButton::middle;
            enqueuePointer(down ? InputEvent::Kind::pointerDown
                                : InputEvent::Kind::pointerUp,
                button, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        }
        case WM_MOUSEWHEEL:
        case WM_MOUSEHWHEEL: {
            POINT point{GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};
            ScreenToClient(window_, &point);
            InputEvent event{.kind = InputEvent::Kind::scroll};
            assignLogicalPoint(event, point.x, point.y);
            const float amount = static_cast<float>(GET_WHEEL_DELTA_WPARAM(wParam)) /
                                 static_cast<float>(WHEEL_DELTA);
            if (message == WM_MOUSEWHEEL)
                event.deltaY = amount;
            else
                event.deltaX = amount;
            event.modifiers = modifiers();
            events_.push_back(event);
            return 0;
        }
        case WM_INPUT:
            if (pointerLocked_)
                enqueueRawInput(reinterpret_cast<HRAWINPUT>(lParam));
            return 0;
        case WM_DROPFILES: {
            const HDROP drop = reinterpret_cast<HDROP>(wParam);
            const UINT count = DragQueryFileW(drop, 0xffffffffU, nullptr, 0);
            for (UINT index = 0; index < count; ++index) {
                const UINT length = DragQueryFileW(drop, index, nullptr, 0);
                std::wstring path(static_cast<std::size_t>(length) + 1, L'\0');
                DragQueryFileW(drop, index, path.data(), length + 1);
                path.resize(length);
                openedDocuments_.emplace_back(path);
            }
            DragFinish(drop);
            return 0;
        }
        case WM_DPICHANGED: {
            const auto* suggested = reinterpret_cast<const RECT*>(lParam);
            SetWindowPos(window_, nullptr, suggested->left, suggested->top,
                suggested->right - suggested->left,
                suggested->bottom - suggested->top, SWP_NOACTIVATE | SWP_NOZORDER);
            return 0;
        }
        default:
            return DefWindowProcW(window_, message, wParam, lParam);
        }
    }

    void assignLogicalPoint(InputEvent& event, LONG x, LONG y) const
    {
        const float density = static_cast<float>(GetDpiForWindow(window_)) /
                              static_cast<float>(USER_DEFAULT_SCREEN_DPI);
        event.x = static_cast<float>(x) / density;
        event.y = static_cast<float>(y) / density;
    }

    void enqueuePointer(InputEvent::Kind kind, InputEvent::PointerButton button,
        LONG x, LONG y)
    {
        InputEvent event{.kind = kind, .button = button, .modifiers = modifiers()};
        assignLogicalPoint(event, x, y);
        events_.push_back(event);
    }

    void enqueueRawInput(HRAWINPUT input)
    {
        UINT size = 0;
        if (GetRawInputData(input, RID_INPUT, nullptr, &size,
                sizeof(RAWINPUTHEADER)) != 0 || size == 0)
            return;
        std::vector<std::byte> storage(size);
        if (GetRawInputData(input, RID_INPUT, storage.data(), &size,
                sizeof(RAWINPUTHEADER)) != size)
            return;
        const auto* raw = reinterpret_cast<const RAWINPUT*>(storage.data());
        if (raw->header.dwType != RIM_TYPEMOUSE ||
            (raw->data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
            return;
        const auto surface = nativeSurface();
        events_.push_back(InputEvent{
            .kind = InputEvent::Kind::pointerMove,
            .x = static_cast<float>(surface.logicalWidth) * 0.5F,
            .y = static_cast<float>(surface.logicalHeight) * 0.5F,
            .deltaX = static_cast<float>(raw->data.mouse.lLastX),
            .deltaY = static_cast<float>(raw->data.mouse.lLastY),
            .modifiers = modifiers()});
    }

    void updatePointerLock()
    {
        const bool shouldLock = pointerLockRequested_ && focused_;
        if (shouldLock == pointerLocked_)
            return;
        RAWINPUTDEVICE device{
            .usUsagePage = 0x01,
            .usUsage = 0x02,
            .dwFlags = shouldLock ? RIDEV_INPUTSINK : RIDEV_REMOVE,
            .hwndTarget = shouldLock ? window_ : nullptr};
        RegisterRawInputDevices(&device, 1, sizeof(device));
        if (shouldLock) {
            savedCursorPositionValid_ =
                GetCursorPos(&savedCursorPosition_) != FALSE;
            RECT clip{};
            GetClientRect(window_, &clip);
            POINT topLeft{clip.left, clip.top};
            POINT bottomRight{clip.right, clip.bottom};
            ClientToScreen(window_, &topLeft);
            ClientToScreen(window_, &bottomRight);
            clip = {topLeft.x, topLeft.y, bottomRight.x, bottomRight.y};
            ClipCursor(&clip);
            while (ShowCursor(FALSE) >= 0) {}
            // Suppress the synthetic center-warp WM_MOUSEMOVE. Gameplay sees
            // only raw relative deltas while locked.
            pointerLocked_ = true;
            SetCursorPos((topLeft.x + bottomRight.x) / 2,
                (topLeft.y + bottomRight.y) / 2);
        } else {
            ClipCursor(nullptr);
            while (ShowCursor(TRUE) < 0) {}
            if (savedCursorPositionValid_) {
                SetCursorPos(savedCursorPosition_.x, savedCursorPosition_.y);
                savedCursorPositionValid_ = false;
            }
        }
        pointerLocked_ = shouldLock;
    }

    void enqueueGamepadButton(std::uint8_t index, InputEvent::GamepadControl control,
                              bool down)
    {
        events_.push_back(InputEvent{
            .kind = down ? InputEvent::Kind::gamepadButtonDown
                         : InputEvent::Kind::gamepadButtonUp,
            .gamepadControl = control,
            .gamepadIndex = index});
    }

    void enqueueGamepadAxis(std::uint8_t index, InputEvent::GamepadControl control,
                            float x, float y = 0.0F)
    {
        events_.push_back(InputEvent{
            .kind = InputEvent::Kind::gamepadAxis,
            .gamepadControl = control,
            .gamepadIndex = index,
            .x = x,
            .y = y});
    }

    static float normalizeThumb(SHORT value)
    {
        return value < 0 ? static_cast<float>(value) / 32768.0F
                         : static_cast<float>(value) / 32767.0F;
    }

    void emitReleasedGamepad(std::uint8_t index, const XINPUT_GAMEPAD& gamepad)
    {
        emitChangedButtons(index, gamepad.wButtons, 0);
        enqueueGamepadAxis(index, InputEvent::GamepadControl::leftStick, 0.0F, 0.0F);
        enqueueGamepadAxis(index, InputEvent::GamepadControl::rightStick, 0.0F, 0.0F);
        enqueueGamepadAxis(index, InputEvent::GamepadControl::leftTrigger, 0.0F);
        enqueueGamepadAxis(index, InputEvent::GamepadControl::rightTrigger, 0.0F);
    }

    void emitChangedButtons(std::uint8_t index, WORD previous, WORD current)
    {
        using Control = InputEvent::GamepadControl;
        static constexpr std::array<std::pair<WORD, Control>, 14> mappings{{
            {XINPUT_GAMEPAD_A, Control::buttonA},
            {XINPUT_GAMEPAD_B, Control::buttonB},
            {XINPUT_GAMEPAD_X, Control::buttonX},
            {XINPUT_GAMEPAD_Y, Control::buttonY},
            {XINPUT_GAMEPAD_LEFT_SHOULDER, Control::leftShoulder},
            {XINPUT_GAMEPAD_RIGHT_SHOULDER, Control::rightShoulder},
            {XINPUT_GAMEPAD_LEFT_THUMB, Control::leftStick},
            {XINPUT_GAMEPAD_RIGHT_THUMB, Control::rightStick},
            {XINPUT_GAMEPAD_START, Control::start},
            {XINPUT_GAMEPAD_BACK, Control::select},
            {XINPUT_GAMEPAD_DPAD_LEFT, Control::dpadLeft},
            {XINPUT_GAMEPAD_DPAD_RIGHT, Control::dpadRight},
            {XINPUT_GAMEPAD_DPAD_UP, Control::dpadUp},
            {XINPUT_GAMEPAD_DPAD_DOWN, Control::dpadDown}}};
        const WORD changed = previous ^ current;
        for (const auto& [mask, control] : mappings) {
            if (changed & mask)
                enqueueGamepadButton(index, control, (current & mask) != 0);
        }
    }

    void emitChangedAxes(std::uint8_t index, const XINPUT_GAMEPAD& previous,
                         const XINPUT_GAMEPAD& current)
    {
        if (previous.sThumbLX != current.sThumbLX || previous.sThumbLY != current.sThumbLY)
            enqueueGamepadAxis(index, InputEvent::GamepadControl::leftStick,
                normalizeThumb(current.sThumbLX), normalizeThumb(current.sThumbLY));
        if (previous.sThumbRX != current.sThumbRX || previous.sThumbRY != current.sThumbRY)
            enqueueGamepadAxis(index, InputEvent::GamepadControl::rightStick,
                normalizeThumb(current.sThumbRX), normalizeThumb(current.sThumbRY));
        if (previous.bLeftTrigger != current.bLeftTrigger)
            enqueueGamepadAxis(index, InputEvent::GamepadControl::leftTrigger,
                static_cast<float>(current.bLeftTrigger) / 255.0F);
        if (previous.bRightTrigger != current.bRightTrigger)
            enqueueGamepadAxis(index, InputEvent::GamepadControl::rightTrigger,
                static_cast<float>(current.bRightTrigger) / 255.0F);
    }

    void pollGamepads()
    {
        for (DWORD index = 0; index < gamepads_.size(); ++index) {
            XINPUT_STATE current{};
            const bool connected = XInputGetState(index, &current) == ERROR_SUCCESS;
            GamepadState& previous = gamepads_[index];
            if (!connected) {
                if (previous.connected)
                    emitReleasedGamepad(static_cast<std::uint8_t>(index), previous.state.Gamepad);
                previous = {};
                continue;
            }
            const XINPUT_GAMEPAD empty{};
            const XINPUT_GAMEPAD& old = previous.connected ? previous.state.Gamepad : empty;
            if (!previous.connected || current.dwPacketNumber != previous.state.dwPacketNumber) {
                emitChangedButtons(static_cast<std::uint8_t>(index), old.wButtons,
                    current.Gamepad.wButtons);
                emitChangedAxes(static_cast<std::uint8_t>(index), old, current.Gamepad);
            }
            previous.connected = true;
            previous.state = current;
        }
    }

    struct GamepadState final {
        bool connected = false;
        XINPUT_STATE state{};
    };

    HWND window_ = nullptr;
    bool visible_ = false;
    bool running_ = true;
    bool focused_ = true;
    bool pointerLockRequested_ = false;
    bool pointerLocked_ = false;
    POINT savedCursorPosition_{};
    bool savedCursorPositionValid_ = false;
    std::vector<InputEvent> events_;
    std::vector<std::filesystem::path> openedDocuments_;
    std::array<GamepadState, XUSER_MAX_COUNT> gamepads_{};
};

} // namespace

std::unique_ptr<Host> createHost(std::uint32_t width, std::uint32_t height,
                                 bool visible)
{
    return std::make_unique<WinHost>(width, height, visible);
}

} // namespace rbx::platform
