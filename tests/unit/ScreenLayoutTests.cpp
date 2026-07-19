#include "rbx/ui/ScreenLayout.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

bool equals(const RBX::UI::ScreenRect& value, float left, float top, float right, float bottom)
{
    return value.left == left && value.top == top && value.right == right && value.bottom == bottom;
}

bool equals(const RBX::UI::LayoutSize& value, float width, float height)
{
    return value.width == width && value.height == height;
}

} // namespace

int main()
{
    using namespace RBX;
    const UI::ScreenRect full{0, 0, 1280, 720};
    const UI::ScreenInsets device{12, 8, 12, 20};
    const UI::ScreenInsets core{0, 44, 0, 0};

    require(equals(UI::resolveScreenRect(full, device, core,
                       SCREEN_INSETS_NONE, false),
                0, 0, 1280, 720),
        "None must expose the full viewport when device clipping is disabled");
    require(equals(UI::resolveScreenRect(full, device, core,
                       SCREEN_INSETS_NONE, true),
                12, 8, 1268, 700),
        "ClipToDeviceSafeArea must constrain full-screen UI");
    require(equals(UI::resolveScreenRect(full, device, core,
                       SCREEN_INSETS_DEVICE_SAFE, true),
                12, 8, 1268, 700),
        "DeviceSafeInsets must use all device edges");
    require(equals(UI::resolveScreenRect(full, device, core,
                       SCREEN_INSETS_CORE_UI_SAFE, true),
                12, 44, 1268, 700),
        "CoreUISafeInsets must combine CoreGui and device-safe bounds");
    require(equals(UI::resolveScreenRect(full, device, core,
                       SCREEN_INSETS_TOPBAR_SAFE, false),
                0, 44, 1280, 720),
        "TopbarSafeInsets must reserve only the CoreGui top bar");

    bool rejected = false;
    try
    {
        static_cast<void>(UI::resolveScreenRect(full, {-1, 0, 0, 0}, core,
            SCREEN_INSETS_NONE, true));
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    require(rejected, "negative insets must be rejected");

    const UI::LayoutSize authored{120, 40};
    const UI::LayoutSize content{260, 90};
    const UI::LayoutSize maximum{200, 300};
    require(equals(UI::resolveAutomaticSize(authored, content, maximum,
                       AUTOMATIC_SIZE_NONE),
                120, 40),
        "None must preserve the authored size");
    require(equals(UI::resolveAutomaticSize(authored, content, maximum,
                       AUTOMATIC_SIZE_X),
                260, 40),
        "X must grow only width; parent bounds do not replace UISizeConstraint");
    require(equals(UI::resolveAutomaticSize(authored, content, maximum,
                       AUTOMATIC_SIZE_Y),
                120, 90),
        "Y must grow only height");
    require(equals(UI::resolveAutomaticSize(authored, {20, 10}, maximum,
                       AUTOMATIC_SIZE_XY),
                120, 40),
        "automatic sizing must not shrink below the authored bounds");
    require(equals(UI::resolveAutomaticSize({-40, -12}, {0, 24}, maximum,
                       AUTOMATIC_SIZE_XY),
                0, 24),
        "transient negative UDim2 results must clamp to a valid AbsoluteSize");

    rejected = false;
    try
    {
        static_cast<void>(UI::resolveAutomaticSize(authored, content, maximum,
            static_cast<AutomaticSize>(99)));
    }
    catch (const std::invalid_argument&)
    {
        rejected = true;
    }
    require(rejected, "unknown AutomaticSize values must be rejected");

    require(UI::resolveGuiState(true, true, false, false) == GUI_STATE_IDLE,
        "an interactable untouched GuiObject must be Idle");
    require(UI::resolveGuiState(true, true, true, false) == GUI_STATE_HOVER,
        "a hovered GuiObject must report Hover");
    require(UI::resolveGuiState(true, true, false, true) == GUI_STATE_PRESS,
        "a captured press must report Press even away from hover");
    require(UI::resolveGuiState(true, false, true, true) == GUI_STATE_NON_INTERACTABLE,
        "inactive state must take precedence over pointer state");
    require(UI::resolveGuiState(false, true, true, true) == GUI_STATE_NON_INTERACTABLE,
        "invisible state must be NonInteractable");
    return 0;
}
