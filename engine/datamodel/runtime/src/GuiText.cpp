#include "v8datamodel/GuiText.h"

#include <cstdlib>

namespace RBX {
namespace {

void appendUtf8(std::string& output, unsigned int codepoint)
{
    if (codepoint <= 0x7f)
        output.push_back(static_cast<char>(codepoint));
    else if (codepoint <= 0x7ff)
    {
        output.push_back(static_cast<char>(0xc0 | (codepoint >> 6)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else if (codepoint <= 0xffff)
    {
        output.push_back(static_cast<char>(0xe0 | (codepoint >> 12)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
    else if (codepoint <= 0x10ffff)
    {
        output.push_back(static_cast<char>(0xf0 | (codepoint >> 18)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3f)));
        output.push_back(static_cast<char>(0x80 | (codepoint & 0x3f)));
    }
}

bool decodeEntity(const std::string& entity, std::string& output)
{
    if (entity == "amp") output.push_back('&');
    else if (entity == "lt") output.push_back('<');
    else if (entity == "gt") output.push_back('>');
    else if (entity == "quot") output.push_back('"');
    else if (entity == "apos") output.push_back('\'');
    else if (entity.size() > 1 && entity[0] == '#')
    {
        char* end = NULL;
        const bool hexadecimal = entity.size() > 2 &&
            (entity[1] == 'x' || entity[1] == 'X');
        const char* digits = entity.c_str() + (hexadecimal ? 2 : 1);
        const unsigned long value = std::strtoul(digits, &end, hexadecimal ? 16 : 10);
        if (!end || *end != '\0' || value > 0x10ffff)
            return false;
        appendUtf8(output, static_cast<unsigned int>(value));
    }
    else return false;
    return true;
}

} // namespace

std::string GuiTextMixin::getContentText() const
{
    if (!richText)
        return text;

    std::string output;
    output.reserve(text.size());
    for (std::string::size_type index = 0; index < text.size();)
    {
        if (text[index] == '<')
        {
            const std::string::size_type close = text.find('>', index + 1);
            if (close != std::string::npos)
            {
                index = close + 1;
                continue;
            }
        }
        if (text[index] == '&')
        {
            const std::string::size_type semicolon = text.find(';', index + 1);
            if (semicolon != std::string::npos && semicolon - index <= 12 &&
                decodeEntity(text.substr(index + 1, semicolon - index - 1), output))
            {
                index = semicolon + 1;
                continue;
            }
        }
        output.push_back(text[index++]);
    }
    return output;
}

} // namespace RBX
