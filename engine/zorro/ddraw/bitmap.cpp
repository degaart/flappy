#include "engine.hpp"

using namespace zorro;

int Bitmap::width() const
{
    return _ddsd.dwWidth;
}

int Bitmap::height() const
{
    return _ddsd.dwHeight;
}

void Bitmap::blt(
    int dstX, int dstY,
    int srcX, int srcY, int srcWidth, int srcHeight,
    int colorKey)
{
    RECT dstRect;
    dstRect.left = dstX;
    dstRect.top = dstY;
    dstRect.right = dstRect.left + srcWidth;
    dstRect.bottom = dstRect.top + srcHeight;

    RECT srcRect;
    srcRect.left = srcX;
    srcRect.top = srcY;
    srcRect.right = srcRect.left + srcWidth;
    srcRect.bottom = srcRect.top + srcHeight;

    auto flags = DDBLT_WAIT;
    if (colorKey != -1)
    {
        flags |= DDBLT_KEYSRC;
    }

    CHECK(_destSurf->Blt(&dstRect, _surface, &srcRect, flags, nullptr));
}

