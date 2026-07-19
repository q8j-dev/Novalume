#pragma once

namespace RBX {
namespace Enums {

// Values and names match the current Player reflection contract.
enum PreferredInput
{
    PREFERRED_INPUT_KEYBOARD_AND_MOUSE = 0,
    PREFERRED_INPUT_GAMEPAD = 1,
    PREFERRED_INPUT_TOUCH = 2,
};

} // namespace Enums

namespace UI {

enum InputFamily
{
    INPUT_FAMILY_UNCHANGED = 0,
    INPUT_FAMILY_KEYBOARD_AND_MOUSE = 1,
    INPUT_FAMILY_GAMEPAD = 2,
    INPUT_FAMILY_TOUCH = 3,
};

[[nodiscard]] Enums::PreferredInput resolvePreferredInput(
    InputFamily family, Enums::PreferredInput current);

} // namespace UI
} // namespace RBX
