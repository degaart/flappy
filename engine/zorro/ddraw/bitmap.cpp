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

static bool clampBlitRect(int dstW, int dstH,
        int srcW, int srcH,
        RECT* dstRect,
        RECT* srcRect)
{
    int dstWidth  = dstRect->right  - dstRect->left;
    int dstHeight = dstRect->bottom - dstRect->top;
    int srcWidth  = srcRect->right  - srcRect->left;
    int srcHeight = srcRect->bottom - srcRect->top;

    if (dstRect->left < 0) {
        int shift = -dstRect->left;
        dstRect->left = 0;
        srcRect->left += shift;
    }
    if (dstRect->top < 0) {
        int shift = -dstRect->top;
        dstRect->top = 0;
        srcRect->top += shift;
    }
    if (dstRect->right > dstW) {
        int excess = dstRect->right - dstW;
        dstRect->right = dstW;
        srcRect->right -= excess;
    }
    if (dstRect->bottom > dstH) {
        int excess = dstRect->bottom - dstH;
        dstRect->bottom = dstH;
        srcRect->bottom -= excess;
    }
    if (srcRect->left < 0) {
        int shift = -srcRect->left;
        srcRect->left = 0;
        dstRect->left += shift;
    }
    if (srcRect->top < 0) {
        int shift = -srcRect->top;
        srcRect->top = 0;
        dstRect->top += shift;
    }
    if (srcRect->right > srcW) {
        int excess = srcRect->right - srcW;
        srcRect->right = srcW;
        dstRect->right -= excess;
    }
    if (srcRect->bottom > srcH) {
        int excess = srcRect->bottom - srcH;
        srcRect->bottom = srcH;
        dstRect->bottom -= excess;
    }

    if (dstRect->left >= dstRect->right ||
        dstRect->top >= dstRect->bottom ||
        srcRect->left >= srcRect->right ||
        srcRect->top >= srcRect->bottom)
    {
        return false;
    }

    return true;
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

    if (!clampBlitRect(_dstWidth, _dstHeight, _ddsd.dwWidth, _ddsd.dwHeight, &dstRect, &srcRect))
    {
        return;
    }

    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);

    auto flags = DDBLT_WAIT;
    if (colorKey != -1)
    {
        flags |= DDBLT_KEYSRCOVERRIDE;

        switch (_bpp)
        {
        case 8:
            fx.ddckSrcColorkey.dwColorSpaceLowValue = colorKey;
            fx.ddckSrcColorkey.dwColorSpaceHighValue = colorKey;
            break;
        case 16:
        case 24:
        case 32:
            fx.ddckSrcColorkey.dwColorSpaceLowValue = Engine::makeRGB(
                _palette[colorKey].peRed,
                _palette[colorKey].peGreen,
                _palette[colorKey].peBlue,
                _pixelFormat
            );
            fx.ddckSrcColorkey.dwColorSpaceHighValue = fx.ddckSrcColorkey.dwColorSpaceLowValue;
            break;
        default:
            panic("Unsupported pixel format");
            break;
        }
    }

    CHECK(_dstSurf->Blt(&dstRect, _surface, &srcRect, flags, &fx));
}

