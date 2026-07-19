#include "GfxRender/TypesetterDynamic.h"

#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{
bool closeEnough(float left, float right, float tolerance)
{
    return std::fabs(left - right) <= tolerance;
}

int fail(const char* message)
{
    std::cerr << "text shaping contract failed: " << message << '\n';
    return 1;
}
}

int main(int argc, char** argv)
{
    if (argc < 2)
        return fail("expected the test font path");

    std::vector<std::string> fallbackPaths;
    for (int i = 2; i < argc; ++i)
        fallbackPaths.push_back(argv[i]);

    RBX::Graphics::TypesetterDynamic typesetter(
        nullptr, nullptr, argv[1], 1.0f, 3, false, fallbackPaths);
    const RBX::Vector2 unconstrained = RBX::Vector2::zero();

    const RBX::Vector2 ligature = typesetter.measure("office", 32.0f, unconstrained, nullptr);
    const RBX::Vector2 splitLigature = typesetter.measure("of\1fice", 32.0f, unconstrained, nullptr);
    if (!(ligature.x < splitLigature.x))
        return fail("OpenType ligature substitution did not affect the measured run");

    const RBX::Vector2 composed = typesetter.measure("\xc3\xa9", 32.0f, unconstrained, nullptr);
    const RBX::Vector2 combining = typesetter.measure("e\xcc\x81", 32.0f, unconstrained, nullptr);
    if (!closeEnough(composed.x, combining.x, 1.0f))
        return fail("combining mark cluster differs from its composed equivalent");

    const RBX::Vector2 malformed = typesetter.measure("\xf0\x28\x8c\x28", 32.0f, unconstrained, nullptr);
    const RBX::Vector2 punctuation = typesetter.measure("((", 32.0f, unconstrained, nullptr);
    if (!(malformed.x > punctuation.x))
        return fail("malformed UTF-8 was discarded instead of producing replacement glyphs");

    if (fallbackPaths.size() >= 2)
    {
        const RBX::Vector2 arabic = typesetter.measure("\xd8\xb3\xd9\x84\xd8\xa7\xd9\x85", 32.0f, unconstrained, nullptr);
        const RBX::Vector2 splitArabic = typesetter.measure("\xd8\xb3\1\xd9\x84\1\xd8\xa7\1\xd9\x85", 32.0f, unconstrained, nullptr);
        if (closeEnough(arabic.x, splitArabic.x, 0.01f))
            return fail("Arabic fallback face did not apply contextual shaping");

        const RBX::Vector2 devanagari = typesetter.measure("\xe0\xa4\x95\xe0\xa5\x8d\xe0\xa4\xb7", 32.0f, unconstrained, nullptr);
        const RBX::Vector2 splitDevanagari = typesetter.measure("\xe0\xa4\x95\1\xe0\xa5\x8d\1\xe0\xa4\xb7", 32.0f, unconstrained, nullptr);
        if (closeEnough(devanagari.x, splitDevanagari.x, 0.01f))
            return fail("Indic fallback face did not apply conjunct shaping");
    }

    const RBX::Vector2 first = typesetter.measure("office e\xcc\x81", 32.0f, unconstrained, nullptr);
    for (int i = 0; i < 1000; ++i)
    {
        const RBX::Vector2 repeated = typesetter.measure("office e\xcc\x81", 32.0f, unconstrained, nullptr);
        if (!closeEnough(first.x, repeated.x, 0.01f) || !closeEnough(first.y, repeated.y, 0.01f))
            return fail("repeated shaping was not deterministic");
    }

    return 0;
}
