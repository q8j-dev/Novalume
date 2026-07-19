#pragma once

#include <string>

namespace RBX {

enum FontWeight
{
    FONT_WEIGHT_THIN = 100,
    FONT_WEIGHT_EXTRA_LIGHT = 200,
    FONT_WEIGHT_LIGHT = 300,
    FONT_WEIGHT_REGULAR = 400,
    FONT_WEIGHT_MEDIUM = 500,
    FONT_WEIGHT_SEMI_BOLD = 600,
    FONT_WEIGHT_BOLD = 700,
    FONT_WEIGHT_EXTRA_BOLD = 800,
    FONT_WEIGHT_HEAVY = 900,
};

enum FontStyle
{
    FONT_STYLE_NORMAL = 0,
    FONT_STYLE_ITALIC = 1,
};

class Font
{
public:
    Font();
    Font(std::string family, FontWeight weight = FONT_WEIGHT_REGULAR,
        FontStyle style = FONT_STYLE_NORMAL);

    static Font fromEnum(int font);

    const std::string& getFamily() const { return family; }
    FontWeight getWeight() const { return weight; }
    FontStyle getStyle() const { return style; }
    bool getBold() const { return weight >= FONT_WEIGHT_BOLD; }

    friend bool operator==(const Font& left, const Font& right);
    friend bool operator!=(const Font& left, const Font& right) { return !(left == right); }
    friend bool operator<(const Font& left, const Font& right);

private:
    std::string family;
    FontWeight weight;
    FontStyle style;
};

} // namespace RBX
