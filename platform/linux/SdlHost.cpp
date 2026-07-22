#include "rbx/platform/Host.h"
#include "rbx/platform/RecentDocuments.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_dialog.h>

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <spawn.h>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <unistd.h>

extern char** environ;

namespace rbx::platform {
namespace {

std::uint32_t modifiers(SDL_Keymod value)
{
    std::uint32_t result = 0;
    result |= (value & SDL_KMOD_SHIFT) ? 1U << 0U : 0U;
    result |= (value & SDL_KMOD_CTRL) ? 1U << 1U : 0U;
    result |= (value & SDL_KMOD_ALT) ? 1U << 2U : 0U;
    result |= (value & SDL_KMOD_GUI) ? 1U << 3U : 0U;
    result |= (value & SDL_KMOD_CAPS) ? 1U << 4U : 0U;
    return result;
}

InputEvent::Key translateKey(SDL_Keycode key)
{
    using Key = InputEvent::Key;
    if (key >= SDLK_0 && key <= SDLK_9)
        return static_cast<Key>(static_cast<unsigned>(Key::zero) + key - SDLK_0);
    if (key >= SDLK_A && key <= SDLK_Z)
        return static_cast<Key>(static_cast<unsigned>(Key::a) + key - SDLK_A);
    if (key >= SDLK_F1 && key <= SDLK_F12)
        return static_cast<Key>(static_cast<unsigned>(Key::f1) + key - SDLK_F1);

    switch (key) {
    case SDLK_BACKSPACE: return Key::backspace;
    case SDLK_TAB: return Key::tab;
    case SDLK_RETURN: return Key::enter;
    case SDLK_ESCAPE: return Key::escape;
    case SDLK_SPACE: return Key::space;
    case SDLK_APOSTROPHE: return Key::quote;
    case SDLK_COMMA: return Key::comma;
    case SDLK_MINUS: return Key::minus;
    case SDLK_PERIOD: return Key::period;
    case SDLK_SLASH: return Key::slash;
    case SDLK_SEMICOLON: return Key::semicolon;
    case SDLK_EQUALS: return Key::equals;
    case SDLK_LEFTBRACKET: return Key::leftBracket;
    case SDLK_BACKSLASH: return Key::backslash;
    case SDLK_RIGHTBRACKET: return Key::rightBracket;
    case SDLK_GRAVE: return Key::backquote;
    case SDLK_LEFT: return Key::left;
    case SDLK_RIGHT: return Key::right;
    case SDLK_UP: return Key::up;
    case SDLK_DOWN: return Key::down;
    case SDLK_LSHIFT: return Key::leftShift;
    case SDLK_RSHIFT: return Key::rightShift;
    case SDLK_LCTRL: return Key::leftControl;
    case SDLK_RCTRL: return Key::rightControl;
    case SDLK_LALT: return Key::leftAlt;
    case SDLK_RALT: return Key::rightAlt;
    case SDLK_LGUI: return Key::leftMeta;
    case SDLK_RGUI: return Key::rightMeta;
    default: return Key::unknown;
    }
}

InputEvent::PointerButton translateButton(std::uint8_t button)
{
    switch (button) {
    case SDL_BUTTON_LEFT: return InputEvent::PointerButton::primary;
    case SDL_BUTTON_RIGHT: return InputEvent::PointerButton::secondary;
    case SDL_BUTTON_MIDDLE: return InputEvent::PointerButton::middle;
    default: return InputEvent::PointerButton::none;
    }
}

std::filesystem::path executablePath()
{
    std::vector<char> buffer(4096);
    for (;;) {
        const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size());
        if (length < 0)
            throw std::runtime_error("failed to resolve the Player executable path");
        if (static_cast<std::size_t>(length) < buffer.size())
            return std::filesystem::path(std::string(buffer.data(), static_cast<std::size_t>(length)));
        buffer.resize(buffer.size() * 2);
    }
}

class SdlHost final : public Host {
public:
    SdlHost(std::uint32_t width, std::uint32_t height, bool visible)
        : visible_(visible)
    {
        if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_GAMEPAD))
            throw std::runtime_error(std::string("failed to initialize SDL: ") + SDL_GetError());

        SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                                SDL_WINDOW_VULKAN;
        if (visible)
            flags |= SDL_WINDOW_HIDDEN;
        else
            flags |= SDL_WINDOW_HIDDEN;
        window_ = SDL_CreateWindow("Roblox", static_cast<int>(width),
            static_cast<int>(height), flags);
        if (!window_) {
            const std::string error = SDL_GetError();
            SDL_Quit();
            throw std::runtime_error("failed to create the Linux Player window: " + error);
        }
        SDL_StartTextInput(window_);
        int gamepadCount = 0;
        if (SDL_JoystickID* gamepads = SDL_GetGamepads(&gamepadCount)) {
            for (int index = 0; index < gamepadCount; ++index)
                openGamepad(gamepads[index]);
            SDL_free(gamepads);
        }
        if (visible_)
            SDL_ShowWindow(window_);
    }

    ~SdlHost() override
    {
        if (window_) {
            for (Controller& controller : controllers_) {
                if (controller.gamepad)
                    SDL_CloseGamepad(controller.gamepad);
            }
            pointerLockRequested_ = false;
            updatePointerLock();
            SDL_StopTextInput(window_);
            SDL_DestroyWindow(window_);
        }
        SDL_Quit();
    }

    NativeSurface nativeSurface() const noexcept override
    {
        int logicalWidth = 0;
        int logicalHeight = 0;
        int pixelWidth = 0;
        int pixelHeight = 0;
        SDL_GetWindowSize(window_, &logicalWidth, &logicalHeight);
        SDL_GetWindowSizeInPixels(window_, &pixelWidth, &pixelHeight);

        const float density = std::max(1.0F, SDL_GetWindowDisplayScale(window_));
        SafeAreaInsets insets;
        SDL_Rect safe{};
        if (SDL_GetWindowSafeArea(window_, &safe)) {
            insets.left = static_cast<float>(std::max(0, safe.x));
            insets.top = static_cast<float>(std::max(0, safe.y));
            insets.right = static_cast<float>(std::max(0, logicalWidth - safe.x - safe.w));
            insets.bottom = static_cast<float>(std::max(0, logicalHeight - safe.y - safe.h));
        }

        const SDL_PropertiesID properties = SDL_GetWindowProperties(window_);
        std::uintptr_t nativeWindow = 0;
        std::uintptr_t nativeDisplay = 0;
        const char* driver = SDL_GetCurrentVideoDriver();
        if (driver && std::string_view(driver) == "wayland") {
            nativeWindow = reinterpret_cast<std::uintptr_t>(SDL_GetPointerProperty(properties,
                SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr));
            nativeDisplay = reinterpret_cast<std::uintptr_t>(SDL_GetPointerProperty(properties,
                SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr));
        } else {
            nativeWindow = static_cast<std::uintptr_t>(SDL_GetNumberProperty(properties,
                SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
            nativeDisplay = reinterpret_cast<std::uintptr_t>(SDL_GetPointerProperty(properties,
                SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr));
        }

        return NativeSurface{
            .window = nativeWindow,
            .display = nativeDisplay,
            .width = static_cast<std::uint32_t>(std::max(0, pixelWidth)),
            .height = static_cast<std::uint32_t>(std::max(0, pixelHeight)),
            .logicalWidth = static_cast<std::uint32_t>(std::max(0, logicalWidth)),
            .logicalHeight = static_cast<std::uint32_t>(std::max(0, logicalHeight)),
            .pixelDensity = density,
            .safeArea = insets};
    }

    std::filesystem::path resourceRoot() const override
    {
        const char* base = SDL_GetBasePath();
        const std::filesystem::path executableDirectory = base
            ? std::filesystem::path(base) : executablePath().parent_path();
        const std::filesystem::path portableResources =
            executableDirectory / "Resources";
        if (std::filesystem::is_directory(portableResources))
            return portableResources;

        // Linux packages keep immutable data under the normal prefix share
        // directory while portable build artifacts retain the adjacent layout.
        return executableDirectory.parent_path() / "share" / "novalume";
    }

    std::filesystem::path writableDataRoot() const override
    {
        char* value = SDL_GetPrefPath("q8j-dev", "Novalume");
        if (!value)
            return {};
        std::filesystem::path result(value);
        SDL_free(value);
        return result;
    }

    std::filesystem::path existingClientSettingsRoot() const override
    {
        if (const char* dataHome = std::getenv("XDG_DATA_HOME"))
            return std::filesystem::path(dataHome) / "Roblox" / "ClientSettings";
        if (const char* userHome = std::getenv("HOME"))
            return std::filesystem::path(userHome) / ".local" / "share" / "Roblox" /
                   "ClientSettings";
        return {};
    }

    bool pumpEvents() override
    {
        SDL_Event event{};
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                events_.push_back(InputEvent{.kind = InputEvent::Kind::nativeCloseRequested});
                running_ = false;
                break;
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                events_.push_back(InputEvent{.kind = InputEvent::Kind::focusGained});
                updatePointerLock();
                break;
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                events_.push_back(InputEvent{.kind = InputEvent::Kind::focusLost});
                updatePointerLock();
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP: {
                const bool down = event.type == SDL_EVENT_KEY_DOWN;
                const char text = down && event.key.key >= 32 && event.key.key <= 126
                    ? static_cast<char>(event.key.key) : 0;
                events_.push_back(InputEvent{
                    .kind = down ? InputEvent::Kind::keyDown : InputEvent::Kind::keyUp,
                    .key = translateKey(event.key.key),
                    .modifiers = modifiers(event.key.mod),
                    .text = text,
                    .repeat = event.key.repeat});
                if (down && !event.key.repeat && event.key.key == SDLK_O &&
                    (event.key.mod & SDL_KMOD_CTRL))
                    requestOpenDocument();
                break;
            }
            case SDL_EVENT_MOUSE_MOTION:
                events_.push_back(InputEvent{
                    .kind = InputEvent::Kind::pointerMove,
                    .x = event.motion.x,
                    .y = event.motion.y,
                    .deltaX = event.motion.xrel,
                    .deltaY = event.motion.yrel,
                    .modifiers = modifiers(SDL_GetModState())});
                break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
                events_.push_back(InputEvent{
                    .kind = event.type == SDL_EVENT_MOUSE_BUTTON_DOWN
                        ? InputEvent::Kind::pointerDown : InputEvent::Kind::pointerUp,
                    .button = translateButton(event.button.button),
                    .x = event.button.x,
                    .y = event.button.y,
                    .modifiers = modifiers(SDL_GetModState())});
                break;
            case SDL_EVENT_MOUSE_WHEEL: {
                const float direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED
                    ? -1.0F : 1.0F;
                events_.push_back(InputEvent{
                    .kind = InputEvent::Kind::scroll,
                    .x = event.wheel.mouse_x,
                    .y = event.wheel.mouse_y,
                    .deltaX = event.wheel.x * direction,
                    .deltaY = event.wheel.y * direction,
                    .modifiers = modifiers(SDL_GetModState())});
                break;
            }
            case SDL_EVENT_DROP_FILE:
                if (event.drop.data)
                    appendOpenedDocument(event.drop.data);
                break;
            case SDL_EVENT_GAMEPAD_ADDED:
                openGamepad(event.gdevice.which);
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                closeGamepad(event.gdevice.which);
                break;
            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                enqueueGamepadButton(event.gbutton);
                break;
            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                enqueueGamepadAxis(event.gaxis);
                break;
            default:
                break;
            }
        }
        return running_;
    }

    std::vector<InputEvent> takeInputEvents() override
    {
        std::vector<InputEvent> result;
        result.swap(events_);
        return result;
    }

    void requestOpenDocument() override
    {
        static constexpr SDL_DialogFileFilter filters[] = {
            {"Roblox places and models", "rbxl;rbxlx;rbxm;rbxmx;rbxlp"},
            {"All files", "*"}};
        SDL_ShowOpenFileDialog(&SdlHost::openDialogCallback, this, window_, filters,
            static_cast<int>(std::size(filters)), nullptr, false);
    }

    std::vector<std::filesystem::path> takeOpenedDocuments() override
    {
        std::scoped_lock lock(documentMutex_);
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
        const std::string executable = executablePath().string();
        const std::string document = path.string();
        char* arguments[] = {
            const_cast<char*>(executable.c_str()),
            const_cast<char*>("--place"),
            const_cast<char*>(document.c_str()),
            nullptr};
        pid_t process = 0;
        const bool launched =
            posix_spawn(&process, executable.c_str(), nullptr, nullptr, arguments, environ) == 0;
        if (launched)
            recordRecentDocument(writableDataRoot(), path);
        return launched;
    }

    bool openExternalUri(std::string_view uri) override
    {
        const std::string value(uri);
        return SDL_OpenURL(value.c_str());
    }

    void setClipboardText(std::string_view text) override
    {
        const std::string value(text);
        SDL_SetClipboardText(value.c_str());
    }

    void setPointerLock(bool locked) override
    {
        pointerLockRequested_ = locked;
        updatePointerLock();
    }

private:
    struct Controller final {
        SDL_JoystickID id = 0;
        SDL_Gamepad* gamepad = nullptr;
        float leftX = 0.0F;
        float leftY = 0.0F;
        float rightX = 0.0F;
        float rightY = 0.0F;
    };

    static void SDLCALL openDialogCallback(void* userdata, const char* const* files, int)
    {
        auto* self = static_cast<SdlHost*>(userdata);
        if (files && files[0])
            self->appendOpenedDocument(files[0]);
    }

    void appendOpenedDocument(const char* path)
    {
        std::scoped_lock lock(documentMutex_);
        openedDocuments_.emplace_back(path);
    }

    void updatePointerLock()
    {
        const bool focused = (SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS) != 0;
        const bool shouldLock = pointerLockRequested_ && focused;
        if (shouldLock == pointerLocked_)
            return;
        if (shouldLock) {
            SDL_GetGlobalMouseState(&savedPointerX_, &savedPointerY_);
            savedPointerPositionValid_ = true;
        }
        if (!SDL_SetWindowRelativeMouseMode(window_, shouldLock))
            return;
        pointerLocked_ = shouldLock;
        if (!shouldLock && savedPointerPositionValid_) {
            SDL_WarpMouseGlobal(savedPointerX_, savedPointerY_);
            savedPointerPositionValid_ = false;
        }
    }

    void openGamepad(SDL_JoystickID id)
    {
        for (const Controller& controller : controllers_) {
            if (controller.id == id)
                return;
        }
        for (Controller& controller : controllers_) {
            if (controller.gamepad)
                continue;
            if (SDL_Gamepad* gamepad = SDL_OpenGamepad(id)) {
                controller = Controller{.id = id, .gamepad = gamepad};
            }
            return;
        }
    }

    void closeGamepad(SDL_JoystickID id)
    {
        for (Controller& controller : controllers_) {
            if (controller.id != id)
                continue;
            if (controller.gamepad)
                SDL_CloseGamepad(controller.gamepad);
            controller = {};
            return;
        }
    }

    std::size_t controllerIndex(SDL_JoystickID id) const
    {
        for (std::size_t index = 0; index < controllers_.size(); ++index) {
            if (controllers_[index].id == id && controllers_[index].gamepad)
                return index;
        }
        return controllers_.size();
    }

    void enqueueGamepadButton(const SDL_GamepadButtonEvent& nativeEvent)
    {
        const std::size_t index = controllerIndex(nativeEvent.which);
        if (index == controllers_.size())
            return;
        using Control = InputEvent::GamepadControl;
        Control control = Control::none;
        switch (static_cast<SDL_GamepadButton>(nativeEvent.button)) {
        case SDL_GAMEPAD_BUTTON_SOUTH: control = Control::buttonA; break;
        case SDL_GAMEPAD_BUTTON_EAST: control = Control::buttonB; break;
        case SDL_GAMEPAD_BUTTON_WEST: control = Control::buttonX; break;
        case SDL_GAMEPAD_BUTTON_NORTH: control = Control::buttonY; break;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER: control = Control::leftShoulder; break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER: control = Control::rightShoulder; break;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: control = Control::leftStick; break;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: control = Control::rightStick; break;
        case SDL_GAMEPAD_BUTTON_START: control = Control::start; break;
        case SDL_GAMEPAD_BUTTON_BACK: control = Control::select; break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: control = Control::dpadLeft; break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: control = Control::dpadRight; break;
        case SDL_GAMEPAD_BUTTON_DPAD_UP: control = Control::dpadUp; break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: control = Control::dpadDown; break;
        default: break;
        }
        if (control == Control::none)
            return;
        events_.push_back(InputEvent{
            .kind = nativeEvent.down ? InputEvent::Kind::gamepadButtonDown
                                     : InputEvent::Kind::gamepadButtonUp,
            .gamepadControl = control,
            .gamepadIndex = static_cast<std::uint8_t>(index)});
    }

    void enqueueGamepadAxis(const SDL_GamepadAxisEvent& nativeEvent)
    {
        const std::size_t index = controllerIndex(nativeEvent.which);
        if (index == controllers_.size())
            return;
        Controller& controller = controllers_[index];
        const float value = nativeEvent.value < 0
            ? static_cast<float>(nativeEvent.value) / 32768.0F
            : static_cast<float>(nativeEvent.value) / 32767.0F;
        using Control = InputEvent::GamepadControl;
        Control control = Control::none;
        switch (static_cast<SDL_GamepadAxis>(nativeEvent.axis)) {
        case SDL_GAMEPAD_AXIS_LEFTX: controller.leftX = value; control = Control::leftStick; break;
        case SDL_GAMEPAD_AXIS_LEFTY: controller.leftY = -value; control = Control::leftStick; break;
        case SDL_GAMEPAD_AXIS_RIGHTX: controller.rightX = value; control = Control::rightStick; break;
        case SDL_GAMEPAD_AXIS_RIGHTY: controller.rightY = -value; control = Control::rightStick; break;
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER: control = Control::leftTrigger; break;
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER: control = Control::rightTrigger; break;
        default: break;
        }
        if (control == Control::none)
            return;
        const bool left = control == Control::leftStick;
        const bool right = control == Control::rightStick;
        events_.push_back(InputEvent{
            .kind = InputEvent::Kind::gamepadAxis,
            .gamepadControl = control,
            .gamepadIndex = static_cast<std::uint8_t>(index),
            .x = left ? controller.leftX : right ? controller.rightX : std::max(0.0F, value),
            .y = left ? controller.leftY : right ? controller.rightY : 0.0F});
    }

    SDL_Window* window_ = nullptr;
    bool visible_ = false;
    bool running_ = true;
    bool pointerLockRequested_ = false;
    bool pointerLocked_ = false;
    bool savedPointerPositionValid_ = false;
    float savedPointerX_ = 0.0F;
    float savedPointerY_ = 0.0F;
    std::vector<InputEvent> events_;
    std::mutex documentMutex_;
    std::vector<std::filesystem::path> openedDocuments_;
    std::array<Controller, 8> controllers_{};
};

} // namespace

std::unique_ptr<Host> createHost(std::uint32_t width, std::uint32_t height, bool visible)
{
    return std::make_unique<SdlHost>(width, height, visible);
}

} // namespace rbx::platform
