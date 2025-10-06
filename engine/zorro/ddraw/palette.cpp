#include "engine.hpp"

using namespace zorro;

int Palette::colorCount() const
{
    return _colors.size();
}

Color<uint8_t> Palette::colorAt(int index) const
{
    return _colors.at(index);
}

const char* Palette::tag() const
{
    return _tag.c_str();
}

