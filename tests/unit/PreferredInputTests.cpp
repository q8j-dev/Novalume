#include "rbx/ui/PreferredInput.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main()
{
    using namespace RBX;
    require(UI::resolvePreferredInput(UI::INPUT_FAMILY_KEYBOARD_AND_MOUSE,
                Enums::PREFERRED_INPUT_TOUCH) == Enums::PREFERRED_INPUT_KEYBOARD_AND_MOUSE,
        "keyboard/mouse input must select KeyboardAndMouse");
    require(UI::resolvePreferredInput(UI::INPUT_FAMILY_GAMEPAD,
                Enums::PREFERRED_INPUT_TOUCH) == Enums::PREFERRED_INPUT_GAMEPAD,
        "gamepad input must select Gamepad");
    require(UI::resolvePreferredInput(UI::INPUT_FAMILY_TOUCH,
                Enums::PREFERRED_INPUT_GAMEPAD) == Enums::PREFERRED_INPUT_TOUCH,
        "touch input must select Touch");
    require(UI::resolvePreferredInput(UI::INPUT_FAMILY_UNCHANGED,
                Enums::PREFERRED_INPUT_GAMEPAD) == Enums::PREFERRED_INPUT_GAMEPAD,
        "non-navigation input must retain the preferred input family");
    return 0;
}
