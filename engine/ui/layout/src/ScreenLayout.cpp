#include "rbx/ui/ScreenLayout.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace RBX::UI {
namespace {

void validate(const LayoutSize& value)
{
    if (!std::isfinite(value.width) || !std::isfinite(value.height))
        throw std::invalid_argument("layout sizes must be finite");
}

void validate(const ScreenRect& rect, const ScreenInsets& insets)
{
    const float values[] = {rect.left, rect.top, rect.right, rect.bottom,
        insets.left, insets.top, insets.right, insets.bottom};
    for (const float value : values)
    {
        if (!std::isfinite(value))
            throw std::invalid_argument("screen rectangles and insets must be finite");
    }
    if (rect.right < rect.left || rect.bottom < rect.top)
        throw std::invalid_argument("screen rectangle is inverted");
    if (insets.left < 0 || insets.top < 0 || insets.right < 0 || insets.bottom < 0)
        throw std::invalid_argument("screen insets must be non-negative");
}

ScreenRect inset(const ScreenRect& rect, const ScreenInsets& insets)
{
    validate(rect, insets);
    const float left = std::min(rect.right, rect.left + insets.left);
    const float top = std::min(rect.bottom, rect.top + insets.top);
    return {
        .left = left,
        .top = top,
        .right = std::max(left, rect.right - insets.right),
        .bottom = std::max(top, rect.bottom - insets.bottom),
    };
}

ScreenRect intersect(const ScreenRect& left, const ScreenRect& right)
{
    const float x0 = std::max(left.left, right.left);
    const float y0 = std::max(left.top, right.top);
    return {
        .left = x0,
        .top = y0,
        .right = std::max(x0, std::min(left.right, right.right)),
        .bottom = std::max(y0, std::min(left.bottom, right.bottom)),
    };
}

} // namespace

LayoutSize resolveAutomaticSize(const LayoutSize& authored,
    const LayoutSize& content, const LayoutSize& parentMaximum, AutomaticSize mode)
{
    validate(authored);
    validate(content);
    validate(parentMaximum);

    // Scale-plus-negative-offset UDim2 sizes are common in genuine CoreGui.
    // During initial attachment their parent can still be zero-sized, making
    // the authored pixel result temporarily negative. AbsoluteSize is clamped
    // per axis until the parent layout becomes available.
    const LayoutSize authoredBounds{
        std::max(0.0f, authored.width), std::max(0.0f, authored.height)};
    const LayoutSize contentBounds{
        std::max(0.0f, content.width), std::max(0.0f, content.height)};
    LayoutSize result = authoredBounds;
    switch (mode)
    {
    case AUTOMATIC_SIZE_NONE:
        break;
    case AUTOMATIC_SIZE_X:
        result.width = std::max(authoredBounds.width, contentBounds.width);
        break;
    case AUTOMATIC_SIZE_Y:
        result.height = std::max(authoredBounds.height, contentBounds.height);
        break;
    case AUTOMATIC_SIZE_XY:
        result.width = std::max(authoredBounds.width, contentBounds.width);
        result.height = std::max(authoredBounds.height, contentBounds.height);
        break;
    default:
        throw std::invalid_argument("unknown AutomaticSize value");
    }
    return result;
}

GuiState resolveGuiState(bool visible, bool active, bool hovered, bool pressed)
{
    if (!visible || !active)
        return GUI_STATE_NON_INTERACTABLE;
    if (pressed)
        return GUI_STATE_PRESS;
    if (hovered)
        return GUI_STATE_HOVER;
    return GUI_STATE_IDLE;
}

ScreenRect resolveScreenRect(const ScreenRect& fullViewport,
    const ScreenInsets& deviceSafeInsets, const ScreenInsets& coreUiInsets,
    ScreenInsetsType mode, bool clipToDeviceSafeArea)
{
    validate(fullViewport, deviceSafeInsets);
    validate(fullViewport, coreUiInsets);
    const ScreenRect deviceSafe = inset(fullViewport, deviceSafeInsets);
    ScreenRect result;
    switch (mode)
    {
    case SCREEN_INSETS_NONE:
        result = fullViewport;
        break;
    case SCREEN_INSETS_DEVICE_SAFE:
        result = deviceSafe;
        break;
    case SCREEN_INSETS_CORE_UI_SAFE:
        result = inset(fullViewport, coreUiInsets);
        break;
    case SCREEN_INSETS_TOPBAR_SAFE:
        result = inset(fullViewport,
            {.left = 0, .top = coreUiInsets.top, .right = 0, .bottom = 0});
        break;
    default:
        throw std::invalid_argument("unknown ScreenInsets value");
    }
    return clipToDeviceSafeArea ? intersect(result, deviceSafe) : result;
}

} // namespace RBX::UI
