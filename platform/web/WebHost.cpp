#include "rbx/platform/Host.h"
#include "rbx/platform/RecentDocuments.h"

#include <emscripten.h>
#include <emscripten/em_asm.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace rbx::platform {
namespace {

void selectDocument()
{
    MAIN_THREAD_EM_ASM({
        const input = document.createElement('input');
        input.type = 'file';
        input.accept = '.rbxl,.rbxlx,.rbxm,.rbxmx,.rbxlp';
        input.onchange = async () => {
            if (!input.files || input.files.length === 0) return;
            const file = input.files[0];
            const name = file.name.replace(/[^A-Za-z0-9._-]/g, '_');
            const path = '/persistent/Roblox/imports/' + name;
            FS.writeFile(path, new Uint8Array(await file.arrayBuffer()));
            FS.syncfs(false, () => Module.ccall('rbxWebDocumentOpened', null,
                ['string'], [path]));
        };
        input.click();
    });
}

int openUri(const char* value)
{
    return MAIN_THREAD_EM_ASM_INT({
        const opened = window.open(UTF8ToString($0), '_blank', 'noopener,noreferrer');
        return opened ? 1 : 0;
    }, value);
}

void writeClipboard(const char* value)
{
    MAIN_THREAD_EM_ASM({
        const text = UTF8ToString($0);
        globalThis.__novalumeClipboard = text;
        if (navigator.clipboard && window.isSecureContext)
            navigator.clipboard.writeText(text).catch(() => {});
    }, value);
}

void requestPointerLock(int locked)
{
    MAIN_THREAD_EM_ASM({
        if ($0) {
            const canvas = document.querySelector('#canvas');
            if (canvas) canvas.requestPointerLock();
        } else if (document.pointerLockElement) {
            document.exitPointerLock();
        }
    }, locked);
}

int launchPath(const char* value)
{
    return MAIN_THREAD_EM_ASM_INT({
        const url = new URL(window.location.href);
        url.searchParams.set('place', UTF8ToString($0));
        window.open(url.toString(), '_blank', 'noopener');
        return 1;
    }, value);
}

class WebHost;
WebHost* activeHost = nullptr;

std::uint32_t modifiers(const EmscriptenKeyboardEvent& event)
{
    return (event.shiftKey ? 1U << 0U : 0U) |
        (event.ctrlKey ? 1U << 1U : 0U) |
        (event.altKey ? 1U << 2U : 0U) |
        (event.metaKey ? 1U << 3U : 0U);
}

InputEvent::Key translateKey(const char* code)
{
    using Key = InputEvent::Key;
    const std::string_view value(code ? code : "");
    if (value.size() == 4 && value.starts_with("Key") && value[3] >= 'A' && value[3] <= 'Z')
        return static_cast<Key>(static_cast<unsigned>(Key::a) + value[3] - 'A');
    if (value.size() == 6 && value.starts_with("Digit") && value[5] >= '0' && value[5] <= '9')
        return static_cast<Key>(static_cast<unsigned>(Key::zero) + value[5] - '0');
    static constexpr std::array<std::pair<std::string_view, Key>, 39> keys{{
        {"Backspace", Key::backspace}, {"Tab", Key::tab}, {"Enter", Key::enter},
        {"Escape", Key::escape}, {"Space", Key::space}, {"Quote", Key::quote},
        {"Comma", Key::comma}, {"Minus", Key::minus}, {"Period", Key::period},
        {"Slash", Key::slash}, {"Semicolon", Key::semicolon}, {"Equal", Key::equals},
        {"BracketLeft", Key::leftBracket}, {"Backslash", Key::backslash},
        {"BracketRight", Key::rightBracket}, {"Backquote", Key::backquote},
        {"ArrowLeft", Key::left}, {"ArrowRight", Key::right}, {"ArrowUp", Key::up},
        {"ArrowDown", Key::down}, {"ShiftLeft", Key::leftShift},
        {"ShiftRight", Key::rightShift}, {"ControlLeft", Key::leftControl},
        {"ControlRight", Key::rightControl}, {"AltLeft", Key::leftAlt},
        {"AltRight", Key::rightAlt}, {"MetaLeft", Key::leftMeta},
        {"MetaRight", Key::rightMeta}, {"F1", Key::f1}, {"F2", Key::f2},
        {"F3", Key::f3}, {"F4", Key::f4}, {"F5", Key::f5}, {"F6", Key::f6},
        {"F7", Key::f7}, {"F8", Key::f8}, {"F9", Key::f9}, {"F10", Key::f10},
        {"F11", Key::f11}}};
    if (value == "F12")
        return Key::f12;
    for (const auto& [native, translated] : keys) {
        if (value == native)
            return translated;
    }
    return Key::unknown;
}

class WebHost final : public Host {
public:
    WebHost(std::uint32_t width, std::uint32_t height, bool)
        : requestedWidth_(width), requestedHeight_(height)
    {
        activeHost = this;
        emscripten_set_canvas_element_size("#canvas", static_cast<int>(width),
            static_cast<int>(height));
        EmscriptenWebGLContextAttributes attributes;
        emscripten_webgl_init_context_attributes(&attributes);
        attributes.alpha = true;
        attributes.depth = true;
        attributes.stencil = true;
        attributes.antialias = false;
        attributes.premultipliedAlpha = false;
        attributes.enableExtensionsByDefault = true;
        attributes.majorVersion = 2;
        attributes.minorVersion = 0;
        attributes.explicitSwapControl = true;
        attributes.renderViaOffscreenBackBuffer = true;
        attributes.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS;
        graphicsContext_ = emscripten_webgl_create_context("#canvas", &attributes);
        if (graphicsContext_ <= 0)
            throw std::runtime_error("browser WebGL 2 context creation failed");
        if (emscripten_webgl_make_context_current(graphicsContext_) != EMSCRIPTEN_RESULT_SUCCESS)
            throw std::runtime_error("browser WebGL 2 context activation failed");
        emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true,
            &WebHost::keyboardCallback);
        emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true,
            &WebHost::keyboardCallback);
        emscripten_set_mousedown_callback("#canvas", this, true, &WebHost::mouseCallback);
        emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true,
            &WebHost::mouseCallback);
        emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true,
            &WebHost::mouseCallback);
        emscripten_set_wheel_callback("#canvas", this, true, &WebHost::wheelCallback);
        emscripten_set_focus_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true,
            &WebHost::focusCallback);
        emscripten_set_blur_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, true,
            &WebHost::focusCallback);
        emscripten_set_visibilitychange_callback(this, true,
            &WebHost::visibilityCallback);
    }

    ~WebHost() override
    {
        if (activeHost == this)
            activeHost = nullptr;
    }

    NativeSurface nativeSurface() const noexcept override
    {
        double logicalWidth = requestedWidth_;
        double logicalHeight = requestedHeight_;
        emscripten_get_element_css_size("#canvas", &logicalWidth, &logicalHeight);
        const double density = emscripten_get_device_pixel_ratio();
        const auto width = static_cast<std::uint32_t>(std::max(1.0, std::round(logicalWidth * density)));
        const auto height = static_cast<std::uint32_t>(std::max(1.0, std::round(logicalHeight * density)));
        emscripten_set_canvas_element_size("#canvas", static_cast<int>(width),
            static_cast<int>(height));
        return NativeSurface{
            .window = reinterpret_cast<std::uintptr_t>("#canvas"),
            .graphicsContext = static_cast<std::uintptr_t>(graphicsContext_),
            .width = width,
            .height = height,
            .logicalWidth = static_cast<std::uint32_t>(std::max(1.0, logicalWidth)),
            .logicalHeight = static_cast<std::uint32_t>(std::max(1.0, logicalHeight)),
            .pixelDensity = static_cast<float>(density),
            .safeArea = {}};
    }

    std::filesystem::path resourceRoot() const override { return "/Resources"; }
    std::filesystem::path writableDataRoot() const override { return "/persistent/Roblox"; }
    bool pumpEvents() override
    {
        pollGamepads();
        return running_;
    }

    std::vector<InputEvent> takeInputEvents() override
    {
        std::scoped_lock lock(mutex_);
        std::vector<InputEvent> result;
        result.swap(events_);
        return result;
    }

    void requestOpenDocument() override { selectDocument(); }

    std::vector<std::filesystem::path> takeOpenedDocuments() override
    {
        std::scoped_lock lock(mutex_);
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
        recordRecentDocument(writableDataRoot(), path);
        return launchPath(path.string().c_str()) != 0;
    }

    bool openExternalUri(std::string_view uri) override
    {
        const std::string value(uri);
        return openUri(value.c_str()) != 0;
    }

    void setClipboardText(std::string_view text) override
    {
        const std::string value(text);
        writeClipboard(value.c_str());
    }

    void setPointerLock(bool locked) override { requestPointerLock(locked ? 1 : 0); }

    void documentOpened(const char* path)
    {
        std::scoped_lock lock(mutex_);
        openedDocuments_.emplace_back(path);
    }

private:
    struct ControllerState final {
        bool connected = false;
        std::array<bool, 14> buttons{};
        float leftX = 0.0F;
        float leftY = 0.0F;
        float rightX = 0.0F;
        float rightY = 0.0F;
        float leftTrigger = 0.0F;
        float rightTrigger = 0.0F;
    };

    void enqueueGamepadButton(std::size_t slot,
        InputEvent::GamepadControl control, bool pressed)
    {
        events_.push_back(InputEvent{
            .kind = pressed ? InputEvent::Kind::gamepadButtonDown
                            : InputEvent::Kind::gamepadButtonUp,
            .gamepadControl = control,
            .gamepadIndex = static_cast<std::uint8_t>(slot),
            .x = pressed ? 1.0F : 0.0F});
    }

    void enqueueGamepadAxis(std::size_t slot,
        InputEvent::GamepadControl control, float x, float y = 0.0F)
    {
        events_.push_back(InputEvent{
            .kind = InputEvent::Kind::gamepadAxis,
            .gamepadControl = control,
            .gamepadIndex = static_cast<std::uint8_t>(slot),
            .x = x,
            .y = y});
    }

    void releaseGamepad(std::size_t slot)
    {
        using Control = InputEvent::GamepadControl;
        static constexpr std::array<Control, 14> controls{{
            Control::buttonA, Control::buttonB, Control::buttonX, Control::buttonY,
            Control::leftShoulder, Control::rightShoulder, Control::leftStick,
            Control::rightStick, Control::start, Control::select, Control::dpadLeft,
            Control::dpadRight, Control::dpadUp, Control::dpadDown}};
        ControllerState& state = controllers_[slot];
        for (std::size_t button = 0; button < controls.size(); ++button) {
            if (state.buttons[button])
                enqueueGamepadButton(slot, controls[button], false);
        }
        enqueueGamepadAxis(slot, Control::leftStick, 0.0F, 0.0F);
        enqueueGamepadAxis(slot, Control::rightStick, 0.0F, 0.0F);
        enqueueGamepadAxis(slot, Control::leftTrigger, 0.0F);
        enqueueGamepadAxis(slot, Control::rightTrigger, 0.0F);
        state = {};
    }

    void pollGamepads()
    {
        if (emscripten_sample_gamepad_data() != EMSCRIPTEN_RESULT_SUCCESS)
            return;
        using Control = InputEvent::GamepadControl;
        static constexpr std::array<Control, 14> controls{{
            Control::buttonA, Control::buttonB, Control::buttonX, Control::buttonY,
            Control::leftShoulder, Control::rightShoulder, Control::leftStick,
            Control::rightStick, Control::start, Control::select, Control::dpadLeft,
            Control::dpadRight, Control::dpadUp, Control::dpadDown}};
        static constexpr std::array<int, 14> buttonIndices{{
            0, 1, 2, 3, 4, 5, 10, 11, 9, 8, 14, 15, 12, 13}};
        const int connected = std::max(0, emscripten_get_num_gamepads());
        std::scoped_lock lock(mutex_);
        for (std::size_t slot = 0; slot < controllers_.size(); ++slot) {
            EmscriptenGamepadEvent gamepad{};
            if (static_cast<int>(slot) >= connected ||
                emscripten_get_gamepad_status(static_cast<int>(slot), &gamepad) !=
                    EMSCRIPTEN_RESULT_SUCCESS || !gamepad.connected) {
                if (controllers_[slot].connected)
                    releaseGamepad(slot);
                continue;
            }
            ControllerState& state = controllers_[slot];
            state.connected = true;
            for (std::size_t button = 0; button < controls.size(); ++button) {
                const int native = buttonIndices[button];
                const bool pressed = native < gamepad.numButtons &&
                    gamepad.digitalButton[native] != 0;
                if (state.buttons[button] != pressed) {
                    state.buttons[button] = pressed;
                    enqueueGamepadButton(slot, controls[button], pressed);
                }
            }
            auto updateAxis = [&](Control control, float x, float y,
                                  float& previousX, float& previousY) {
                if (std::abs(x - previousX) > 0.0001F ||
                    std::abs(y - previousY) > 0.0001F) {
                    previousX = x;
                    previousY = y;
                    enqueueGamepadAxis(slot, control, x, y);
                }
            };
            const float leftX = gamepad.numAxes > 0 ? static_cast<float>(gamepad.axis[0]) : 0.0F;
            const float leftY = gamepad.numAxes > 1 ? -static_cast<float>(gamepad.axis[1]) : 0.0F;
            const float rightX = gamepad.numAxes > 2 ? static_cast<float>(gamepad.axis[2]) : 0.0F;
            const float rightY = gamepad.numAxes > 3 ? -static_cast<float>(gamepad.axis[3]) : 0.0F;
            updateAxis(Control::leftStick, leftX, leftY, state.leftX, state.leftY);
            updateAxis(Control::rightStick, rightX, rightY, state.rightX, state.rightY);
            const float leftTrigger = gamepad.numButtons > 6
                ? static_cast<float>(gamepad.analogButton[6]) : 0.0F;
            const float rightTrigger = gamepad.numButtons > 7
                ? static_cast<float>(gamepad.analogButton[7]) : 0.0F;
            if (std::abs(leftTrigger - state.leftTrigger) > 0.0001F) {
                state.leftTrigger = leftTrigger;
                enqueueGamepadAxis(slot, Control::leftTrigger, leftTrigger);
            }
            if (std::abs(rightTrigger - state.rightTrigger) > 0.0001F) {
                state.rightTrigger = rightTrigger;
                enqueueGamepadAxis(slot, Control::rightTrigger, rightTrigger);
            }
        }
    }

    static EM_BOOL keyboardCallback(int type, const EmscriptenKeyboardEvent* event,
        void* userData)
    {
        auto& self = *static_cast<WebHost*>(userData);
        const bool down = type == EMSCRIPTEN_EVENT_KEYDOWN;
        InputEvent translated{
            .kind = down ? InputEvent::Kind::keyDown : InputEvent::Kind::keyUp,
            .key = translateKey(event->code),
            .modifiers = modifiers(*event),
            .text = static_cast<char>(down && event->key[0] && !event->key[1] ? event->key[0] : 0),
            .repeat = event->repeat};
        std::scoped_lock lock(self.mutex_);
        self.events_.push_back(translated);
        return translated.key != InputEvent::Key::unknown;
    }

    static EM_BOOL mouseCallback(int type, const EmscriptenMouseEvent* event, void* userData)
    {
        auto& self = *static_cast<WebHost*>(userData);
        InputEvent translated;
        translated.kind = type == EMSCRIPTEN_EVENT_MOUSEDOWN ? InputEvent::Kind::pointerDown :
            type == EMSCRIPTEN_EVENT_MOUSEUP ? InputEvent::Kind::pointerUp :
            InputEvent::Kind::pointerMove;
        translated.x = static_cast<float>(event->targetX);
        translated.y = static_cast<float>(event->targetY);
        translated.deltaX = static_cast<float>(event->movementX);
        translated.deltaY = static_cast<float>(event->movementY);
        translated.modifiers = (event->shiftKey ? 1U << 0U : 0U) |
            (event->ctrlKey ? 1U << 1U : 0U) | (event->altKey ? 1U << 2U : 0U) |
            (event->metaKey ? 1U << 3U : 0U);
        translated.button = event->button == 0 ? InputEvent::PointerButton::primary :
            event->button == 1 ? InputEvent::PointerButton::middle :
            event->button == 2 ? InputEvent::PointerButton::secondary :
            InputEvent::PointerButton::none;
        std::scoped_lock lock(self.mutex_);
        self.events_.push_back(translated);
        return true;
    }

    static EM_BOOL wheelCallback(int, const EmscriptenWheelEvent* event, void* userData)
    {
        auto& self = *static_cast<WebHost*>(userData);
        InputEvent translated{
            .kind = InputEvent::Kind::scroll,
            .x = static_cast<float>(event->mouse.targetX),
            .y = static_cast<float>(event->mouse.targetY),
            .deltaX = static_cast<float>(-event->deltaX),
            .deltaY = static_cast<float>(-event->deltaY)};
        std::scoped_lock lock(self.mutex_);
        self.events_.push_back(translated);
        return true;
    }

    static EM_BOOL focusCallback(int type, const EmscriptenFocusEvent*, void* userData)
    {
        auto& self = *static_cast<WebHost*>(userData);
        std::scoped_lock lock(self.mutex_);
        self.events_.push_back(InputEvent{.kind = type == EMSCRIPTEN_EVENT_FOCUS ?
            InputEvent::Kind::focusGained : InputEvent::Kind::focusLost});
        return false;
    }

    static EM_BOOL visibilityCallback(int, const EmscriptenVisibilityChangeEvent* event,
        void* userData)
    {
        auto& self = *static_cast<WebHost*>(userData);
        std::scoped_lock lock(self.mutex_);
        self.events_.push_back(InputEvent{.kind = event->hidden
            ? InputEvent::Kind::focusLost : InputEvent::Kind::focusGained});
        return false;
    }

    std::uint32_t requestedWidth_;
    std::uint32_t requestedHeight_;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE graphicsContext_ = 0;
    bool running_ = true;
    mutable std::mutex mutex_;
    std::vector<InputEvent> events_;
    std::vector<std::filesystem::path> openedDocuments_;
    std::array<ControllerState, 4> controllers_{};
};

}

extern "C" EMSCRIPTEN_KEEPALIVE void rbxWebDocumentOpened(const char* path)
{
    if (activeHost)
        activeHost->documentOpened(path);
}

std::unique_ptr<Host> createHost(std::uint32_t width, std::uint32_t height, bool visible)
{
    return std::make_unique<WebHost>(width, height, visible);
}

}
