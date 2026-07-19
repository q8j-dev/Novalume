#pragma once

#include <cstdint>

namespace RBX {

// Names and values are kept serialization-stable for current Player places.
enum ScreenInsetsType : int
{
    SCREEN_INSETS_NONE = 0,
    SCREEN_INSETS_DEVICE_SAFE = 1,
    SCREEN_INSETS_CORE_UI_SAFE = 2,
    SCREEN_INSETS_TOPBAR_SAFE = 3,
};

enum SafeAreaCompatMode
{
    SAFE_AREA_COMPAT_NONE = 0,
    SAFE_AREA_COMPAT_FULLSCREEN_EXTENSION = 1,
};

// Serialization values match the Player-facing Enum.AutomaticSize contract.
enum AutomaticSize : int
{
    AUTOMATIC_SIZE_NONE = 0,
    AUTOMATIC_SIZE_X = 1,
    AUTOMATIC_SIZE_Y = 2,
    AUTOMATIC_SIZE_XY = 3,
};

enum GuiState : int
{
    GUI_STATE_IDLE = 0,
    GUI_STATE_HOVER = 1,
    GUI_STATE_PRESS = 2,
    GUI_STATE_NON_INTERACTABLE = 3,
};

namespace UI {

struct ScreenRect
{
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;
};

struct ScreenInsets
{
    float left = 0;
    float top = 0;
    float right = 0;
    float bottom = 0;
};

struct LayoutSize
{
    float width = 0;
    float height = 0;
};

// Automatic axes grow to contain content, retain the authored size as a lower
// bound, and cannot exceed the available parent extent.
[[nodiscard]] LayoutSize resolveAutomaticSize(const LayoutSize& authored,
    const LayoutSize& content, const LayoutSize& parentMaximum, AutomaticSize mode);

[[nodiscard]] GuiState resolveGuiState(bool visible, bool active, bool hovered, bool pressed);

[[nodiscard]] ScreenRect resolveScreenRect(const ScreenRect& fullViewport,
    const ScreenInsets& deviceSafeInsets, const ScreenInsets& coreUiInsets,
    ScreenInsetsType mode, bool clipToDeviceSafeArea);

} // namespace UI
} // namespace RBX
