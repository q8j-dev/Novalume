#include "Util/Font.h"

#include "Reflection/EnumConverter.h"
#include "Reflection/Type.h"

#include <stdexcept>
#include <tuple>

namespace RBX {
namespace {
const char* const kSourceSans = "rbxasset://fonts/families/SourceSansPro.json";
const char* const kLegacyArial = "rbxasset://fonts/families/LegacyArial.json";
const char* const kBuilderSans = "rbxasset://fonts/families/BuilderSans.json";
const char* const kBuilderIconsRegular =
    "rbxasset://LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/Font/BuilderIcons-Regular.ttf";
const char* const kBuilderIconsFilled =
    "rbxasset://LuaPackages/Packages/_Index/BuilderIcons/BuilderIcons/Font/BuilderIcons-Filled.ttf";
const char* const kFamilies = "rbxasset://fonts/families/";

Font makeFamilyFont(const char* name, FontWeight weight = FONT_WEIGHT_REGULAR,
    FontStyle style = FONT_STYLE_NORMAL)
{
    return Font(std::string(kFamilies) + name + ".json", weight, style);
}
}

Font::Font()
    : family(kSourceSans)
    , weight(FONT_WEIGHT_REGULAR)
    , style(FONT_STYLE_NORMAL)
{
}

Font::Font(std::string family, FontWeight weight, FontStyle style)
    : family(std::move(family))
    , weight(weight)
    , style(style)
{
    if (this->family.empty())
        throw std::invalid_argument("family must not be empty");
}

Font Font::fromEnum(int font)
{
    switch (font)
    {
    case 0: return Font(kLegacyArial);
    case 1: return Font(kLegacyArial);
    case 2: return Font(kLegacyArial, FONT_WEIGHT_BOLD);
    case 3: return Font(kSourceSans);
    case 4: return Font(kSourceSans, FONT_WEIGHT_BOLD);
    case 5: return Font(kSourceSans, FONT_WEIGHT_LIGHT);
    case 6: return Font(kSourceSans, FONT_WEIGHT_REGULAR, FONT_STYLE_ITALIC);
    case 7: return makeFamilyFont("AccanthisADFStd");
    case 8: return makeFamilyFont("Guru");
    case 9: return makeFamilyFont("ComicNeueAngular", FONT_WEIGHT_BOLD);
    case 10: return makeFamilyFont("Inconsolata");
    case 11: return makeFamilyFont("HighwayGothic");
    case 12: return makeFamilyFont("Zekton");
    case 13: return makeFamilyFont("PressStart2P");
    case 14: return makeFamilyFont("Balthazar");
    case 15: return makeFamilyFont("RomanAntique");
    case 16: return Font(kSourceSans, FONT_WEIGHT_SEMI_BOLD);
    case 17: return makeFamilyFont("Montserrat");
    case 18: return makeFamilyFont("Montserrat", FONT_WEIGHT_MEDIUM);
    case 19: return makeFamilyFont("Montserrat", FONT_WEIGHT_BOLD);
    case 20: return makeFamilyFont("Montserrat", FONT_WEIGHT_HEAVY);
    case 21: return makeFamilyFont("AmaticSC");
    case 22: return makeFamilyFont("Bangers");
    case 23: return makeFamilyFont("Creepster");
    case 24: return makeFamilyFont("DenkOne");
    case 25: return makeFamilyFont("Fondamento");
    case 26: return makeFamilyFont("FredokaOne");
    case 27: return makeFamilyFont("GrenzeGotisch");
    case 28: return makeFamilyFont("IndieFlower");
    case 29: return makeFamilyFont("JosefinSans");
    case 30: return makeFamilyFont("Jura");
    case 31: return makeFamilyFont("Kalam");
    case 32: return makeFamilyFont("LuckiestGuy");
    case 33: return makeFamilyFont("Merriweather");
    case 34: return makeFamilyFont("Michroma");
    case 35: return makeFamilyFont("Nunito");
    case 36: return makeFamilyFont("Oswald");
    case 37: return makeFamilyFont("PatrickHand");
    case 38: return makeFamilyFont("PermanentMarker");
    case 39: return makeFamilyFont("Roboto");
    case 40: return makeFamilyFont("RobotoCondensed");
    case 41: return makeFamilyFont("RobotoMono");
    case 42: return makeFamilyFont("Sarpanch");
    case 43: return makeFamilyFont("SpecialElite");
    case 44: return makeFamilyFont("TitilliumWeb");
    case 45: return makeFamilyFont("Ubuntu");
    case 46: return Font(kBuilderSans);
    case 47: return Font(kBuilderSans, FONT_WEIGHT_MEDIUM);
    case 48: return Font(kBuilderSans, FONT_WEIGHT_BOLD);
    case 49: return Font(kBuilderSans, FONT_WEIGHT_EXTRA_BOLD);
    case 50: return makeFamilyFont("Arimo");
    case 51: return makeFamilyFont("Arimo", FONT_WEIGHT_BOLD);
    case 100: return Font(kSourceSans);
    case 101: return Font(kBuilderIconsRegular);
    case 102: return Font(kBuilderIconsFilled);
    default: throw std::invalid_argument("unknown Enum.Font value");
    }
}

bool operator==(const Font& left, const Font& right)
{
    return left.family == right.family && left.weight == right.weight && left.style == right.style;
}

bool operator<(const Font& left, const Font& right)
{
    return std::tie(left.family, left.weight, left.style) <
        std::tie(right.family, right.weight, right.style);
}

namespace Reflection {
template<> EnumDesc<RBX::FontWeight>::EnumDesc()
    : EnumDescriptor("FontWeight")
{
    addPair(FONT_WEIGHT_THIN, "Thin");
    addPair(FONT_WEIGHT_EXTRA_LIGHT, "ExtraLight");
    addPair(FONT_WEIGHT_LIGHT, "Light");
    addPair(FONT_WEIGHT_REGULAR, "Regular");
    addPair(FONT_WEIGHT_MEDIUM, "Medium");
    addPair(FONT_WEIGHT_SEMI_BOLD, "SemiBold");
    addPair(FONT_WEIGHT_BOLD, "Bold");
    addPair(FONT_WEIGHT_EXTRA_BOLD, "ExtraBold");
    addPair(FONT_WEIGHT_HEAVY, "Heavy");
}

template<> EnumDesc<RBX::FontStyle>::EnumDesc()
    : EnumDescriptor("FontStyle")
{
    addPair(FONT_STYLE_NORMAL, "Normal");
    addPair(FONT_STYLE_ITALIC, "Italic");
}

template<> const Type& Type::getSingleton<RBX::Font>()
{
    static TType<RBX::Font> type("Font");
    return type;
}

} // namespace Reflection
} // namespace RBX
