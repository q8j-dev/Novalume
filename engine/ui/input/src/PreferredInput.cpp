#include "rbx/ui/PreferredInput.h"

namespace RBX {
namespace UI {

Enums::PreferredInput resolvePreferredInput(InputFamily family, Enums::PreferredInput current)
{
    switch (family)
    {
    case INPUT_FAMILY_KEYBOARD_AND_MOUSE:
        return Enums::PREFERRED_INPUT_KEYBOARD_AND_MOUSE;
    case INPUT_FAMILY_GAMEPAD:
        return Enums::PREFERRED_INPUT_GAMEPAD;
    case INPUT_FAMILY_TOUCH:
        return Enums::PREFERRED_INPUT_TOUCH;
    case INPUT_FAMILY_UNCHANGED:
    default:
        return current;
    }
}

} // namespace UI
} // namespace RBX
