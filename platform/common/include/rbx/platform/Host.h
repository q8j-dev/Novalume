#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <span>
#include <string_view>
#include <vector>

namespace rbx::platform {

struct SafeAreaInsets final {
    float left = 0.0F;
    float top = 0.0F;
    float right = 0.0F;
    float bottom = 0.0F;
};

struct NativeSurface final {
    std::uintptr_t window = 0;
    std::uintptr_t display = 0;
    std::uintptr_t graphicsContext = 0;
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t logicalWidth = 0;
    std::uint32_t logicalHeight = 0;
    float pixelDensity = 1.0F;
    // Insets are expressed in logical display units, like input and CoreGui.
    SafeAreaInsets safeArea;
};

// Platform adapters translate native events into this small, engine-neutral
// vocabulary. Roblox data-model types deliberately do not leak into Host so
// every future platform can share the Player-side input bridge.
struct InputEvent final {
    enum class Kind : std::uint8_t {
        keyDown,
        keyUp,
        pointerMove,
        pointerDown,
        pointerUp,
        scroll,
        gamepadButtonDown,
        gamepadButtonUp,
        gamepadAxis,
        focusGained,
        focusLost,
        nativeCloseRequested
    };

    enum class Key : std::uint16_t {
        unknown,
        backspace,
        tab,
        enter,
        escape,
        space,
        quote,
        comma,
        minus,
        period,
        slash,
        zero,
        one,
        two,
        three,
        four,
        five,
        six,
        seven,
        eight,
        nine,
        semicolon,
        equals,
        leftBracket,
        backslash,
        rightBracket,
        backquote,
        a,
        b,
        c,
        d,
        e,
        f,
        g,
        h,
        i,
        j,
        k,
        l,
        m,
        n,
        o,
        p,
        q,
        r,
        s,
        t,
        u,
        v,
        w,
        x,
        y,
        z,
        left,
        right,
        up,
        down,
        leftShift,
        rightShift,
        leftControl,
        rightControl,
        leftAlt,
        rightAlt,
        leftMeta,
        rightMeta,
        f1,
        f2,
        f3,
        f4,
        f5,
        f6,
        f7,
        f8,
        f9,
        f10,
        f11,
        f12
    };

    enum class PointerButton : std::uint8_t { none, primary, secondary, middle };

    enum class GamepadControl : std::uint8_t {
        none,
        buttonA,
        buttonB,
        buttonX,
        buttonY,
        leftShoulder,
        rightShoulder,
        leftTrigger,
        rightTrigger,
        leftStick,
        rightStick,
        start,
        select,
        dpadLeft,
        dpadRight,
        dpadUp,
        dpadDown
    };

    Kind kind = Kind::pointerMove;
    Key key = Key::unknown;
    PointerButton button = PointerButton::none;
    GamepadControl gamepadControl = GamepadControl::none;
    std::uint8_t gamepadIndex = 0;
    float x = 0.0F;
    float y = 0.0F;
    float deltaX = 0.0F;
    float deltaY = 0.0F;
    std::uint32_t modifiers = 0;
    char text = 0;
    bool repeat = false;
};

enum class LifecycleEvent : std::uint8_t {
    foreground,
    background,
    suspend,
    resume,
    surfaceLost,
    surfaceCreated,
    quit
};

class Host {
public:
    virtual ~Host() = default;
    [[nodiscard]] virtual NativeSurface nativeSurface() const noexcept = 0;
    [[nodiscard]] virtual std::filesystem::path resourceRoot() const = 0;
    [[nodiscard]] virtual std::filesystem::path writableDataRoot() const = 0;
    // Existing Roblox client settings are an optional, read-only bootstrap
    // cache. Platform adapters expose their native location; shared Player
    // code owns parsing and remains functional when the cache is absent.
    [[nodiscard]] virtual std::filesystem::path existingClientSettingsRoot() const {
        return {};
    }
    virtual bool pumpEvents() = 0;
    [[nodiscard]] virtual std::vector<InputEvent> takeInputEvents() = 0;
    // Launcher document selection stays behind the host boundary because each
    // product platform owns its picker and application activation contract.
    // Paths cross back as standard filesystem values; no native UI type leaks
    // into the shared Player.
    virtual void requestOpenDocument() = 0;
    [[nodiscard]] virtual std::vector<std::filesystem::path>
        takeOpenedDocuments() = 0;
    [[nodiscard]] virtual std::vector<std::filesystem::path>
        recentDocuments() const = 0;
    // Starts the selected document in a fresh Player session. The launcher
    // exits only after the platform confirms that the session was created.
    virtual bool launchDocument(const std::filesystem::path& path) = 0;
    [[nodiscard]] virtual bool openExternalUri(std::string_view uri) = 0;
    virtual void setClipboardText(std::string_view text) = 0;
    // A locked pointer uses native relative motion while the engine keeps its
    // software cursor at the lock point. Hosts must restore the system cursor
    // and pointer association when focus is lost or the lock is released.
    virtual void setPointerLock(bool locked) = 0;
};

// Each product platform provides exactly one production host factory through
// its adapter target. Shared Player code must not select an OS-specific host.
[[nodiscard]] std::unique_ptr<Host> createHost(std::uint32_t width,
                                               std::uint32_t height,
                                               bool visible);

} // namespace rbx::platform
