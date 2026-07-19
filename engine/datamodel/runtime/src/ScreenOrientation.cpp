#include "V8DataModel/ScreenOrientation.h"

#include "reflection/enumconverter.h"

namespace RBX {
namespace Reflection {

template<> EnumDesc<Enums::ScreenOrientation>::EnumDesc()
    : EnumDescriptor("ScreenOrientation")
{
    addPair(Enums::SCREEN_ORIENTATION_LANDSCAPE_LEFT, "LandscapeLeft");
    addPair(Enums::SCREEN_ORIENTATION_LANDSCAPE_RIGHT, "LandscapeRight");
    addPair(Enums::SCREEN_ORIENTATION_LANDSCAPE_SENSOR, "LandscapeSensor");
    addPair(Enums::SCREEN_ORIENTATION_PORTRAIT, "Portrait");
    addPair(Enums::SCREEN_ORIENTATION_SENSOR, "Sensor");
}

template<> Enums::ScreenOrientation& Variant::convert<Enums::ScreenOrientation>()
{
    return genericConvert<Enums::ScreenOrientation>();
}

} // namespace Reflection

template<> bool StringConverter<Enums::ScreenOrientation>::convertToValue(
    const std::string& text, Enums::ScreenOrientation& value)
{
    return Reflection::EnumDesc<Enums::ScreenOrientation>::singleton()
        .convertToValue(text.c_str(), value);
}

} // namespace RBX
