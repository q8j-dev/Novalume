#include "util/Path2DControlPoint.h"

#include "reflection/Property.h"
#include "reflection/Type.h"
#include "util/Utilities.h"
#include "v8xml/XmlElement.h"

namespace RBX {
namespace {

std::string encode(const Path2DControlPoint& value)
{
    return StringConverter<UDim2>::convertToString(value.position) + " | " +
        StringConverter<UDim2>::convertToString(value.leftTangent) + " | " +
        StringConverter<UDim2>::convertToString(value.rightTangent);
}

bool decode(const std::string& text, Path2DControlPoint& value)
{
    const std::string::size_type first = text.find('|');
    const std::string::size_type second = first == std::string::npos
        ? std::string::npos : text.find('|', first + 1);
    if (first == std::string::npos || second == std::string::npos)
        return false;
    return StringConverter<UDim2>::convertToValue(text.substr(0, first), value.position) &&
        StringConverter<UDim2>::convertToValue(
            text.substr(first + 1, second - first - 1), value.leftTangent) &&
        StringConverter<UDim2>::convertToValue(text.substr(second + 1), value.rightTangent);
}

} // namespace

namespace Reflection {

template<> int TypedPropertyDescriptor<Path2DControlPoint>::getDataSize(
    const DescribedBase*) const
{
    return sizeof(Path2DControlPoint);
}

template<> void TypedPropertyDescriptor<Path2DControlPoint>::readValue(
    DescribedBase* instance, const XmlElement* element, IReferenceBinder&) const
{
    std::string text;
    element->getValue(text);
    Path2DControlPoint value;
    if (decode(text, value))
        setValue(instance, value);
}

template<> void TypedPropertyDescriptor<Path2DControlPoint>::writeValue(
    const DescribedBase* instance, XmlElement* element) const
{
    element->setValue(encode(getValue(instance)));
}

template<> bool TypedPropertyDescriptor<Path2DControlPoint>::hasStringValue() const
{
    return true;
}

template<> std::string TypedPropertyDescriptor<Path2DControlPoint>::getStringValue(
    const DescribedBase* instance) const
{
    return encode(getValue(instance));
}

template<> bool TypedPropertyDescriptor<Path2DControlPoint>::setStringValue(
    DescribedBase* instance, const std::string& text) const
{
    Path2DControlPoint value;
    if (!decode(text, value))
        return false;
    setValue(instance, value);
    return true;
}

template<> Path2DControlPoint& Variant::convert<Path2DControlPoint>()
{
    return genericConvert<Path2DControlPoint>();
}

template<> const Type& Type::getSingleton<Path2DControlPoint>()
{
    static TType<Path2DControlPoint> type("Path2DControlPoint");
    return type;
}

} // namespace Reflection

template<> bool StringConverter<Path2DControlPoint>::convertToValue(
    const std::string& text, Path2DControlPoint& value)
{
    return decode(text, value);
}

template<> std::string StringConverter<Path2DControlPoint>::convertToString(
    const Path2DControlPoint& value)
{
    return encode(value);
}

} // namespace RBX
