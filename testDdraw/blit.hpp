#include <stdint.h>
#include <stddef.h>

struct Surface
{
    int width;
    int height;
    int pitch;
    int bpp;      /* 8, 16, 24, or 32 */
    uint8_t* data;
    uint32_t* palette;  /* only for 8bpp images, always 256 entries */
};

static inline int min_int(int a, int b) { return a < b ? a : b; }
static inline int max_int(int a, int b) { return a > b ? a : b; }

// 8bpp source
void blit_8_to_8(Surface* dst, int dstX, int dstY,
                 Surface* src, int srcX, int srcY,
                 int width, int height, uint32_t* colorKey);

void blit_8_to_16(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey);

void blit_8_to_24(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey);

void blit_8_to_32(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey);

// 16bpp source
void blit_16_to_8(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey);

void blit_16_to_16(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey);

void blit_16_to_24(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey);

void blit_16_to_32(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey);

// 24bpp source
void blit_24_to_8(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey);

void blit_24_to_16(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey);

void blit_24_to_24(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey);

void blit_24_to_32(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey);

// 32bpp source
void blit_32_to_8(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey);

void blit_32_to_16(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey);

void blit_32_to_24(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey);

void blit_32_to_32(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey);

void blit(Surface* dst, int dstX, int dstY, int dstWidth, int dstHeight,
          Surface* src, int srcX, int srcY, int srcWidth, int srcHeight,
          uint32_t* colorKey)
{
    if (!dst || !src) return;

    // Clip source rectangle to source surface
    if (srcX < 0) { dstX -= srcX; srcWidth += srcX; srcX = 0; }
    if (srcY < 0) { dstY -= srcY; srcHeight += srcY; srcY = 0; }
    if (srcX + srcWidth > src->width)  srcWidth  = src->width  - srcX;
    if (srcY + srcHeight > src->height) srcHeight = src->height - srcY;

    // Clip destination rectangle to destination surface
    if (dstX < 0) { srcX -= dstX; srcWidth += dstX; dstX = 0; }
    if (dstY < 0) { srcY -= dstY; srcHeight += dstY; dstY = 0; }
    if (dstX + srcWidth > dst->width)  srcWidth  = dst->width  - dstX;
    if (dstY + srcHeight > dst->height) srcHeight = dst->height - dstY;

    // Nothing to blit
    if (srcWidth <= 0 || srcHeight <= 0) return;

    // Dispatch based on src->bpp and dst->bpp
    switch (src->bpp)
    {
        case 8:
            switch (dst->bpp)
            {
                case 8:
                    // blit_8_to_8(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 16:
                    // blit_8_to_16(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 24:
                    // blit_8_to_24(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 32:
                    // blit_8_to_32(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
            }
            break;
        case 16:
            switch (dst->bpp)
            {
                case 8:
                    // blit_16_to_8(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 16:
                    // blit_16_to_16(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 24:
                    // blit_16_to_24(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 32:
                    // blit_16_to_32(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
            }
            break;
        case 24:
            switch (dst->bpp)
            {
                case 8:
                    // blit_24_to_8(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 16:
                    // blit_24_to_16(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 24:
                    // blit_24_to_24(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 32:
                    // blit_24_to_32(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
            }
            break;
        case 32:
            switch (dst->bpp)
            {
                case 8:
                    // blit_32_to_8(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 16:
                    // blit_32_to_16(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 24:
                    // blit_32_to_24(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
                case 32:
                    // blit_32_to_32(dst, dstX, dstY, src, srcX, srcY, srcWidth, srcHeight, colorKey);
                    break;
            }
            break;
    }
}

// 8bpp -> 8bpp
void blit_8_to_8(Surface* dst, int dstX, int dstY,
                 Surface* src, int srcX, int srcY,
                 int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRow = src->data + (srcY + y) * src->pitch + srcX;
        uint8_t* dstRow = dst->data + (dstY + y) * dst->pitch + dstX;

        if (colorKey)
        {
            uint8_t keyIndex = 0;
            // Map the colorKey into a palette index if possible
            for (int i = 0; i < 256; ++i)
            {
                if (src->palette[i] == *colorKey) { keyIndex = (uint8_t)i; break; }
            }

            for (int x = 0; x < width; ++x)
            {
                uint8_t pixel = srcRow[x];
                if (pixel != keyIndex)
                    dstRow[x] = pixel;
            }
        }
        else
        {
            memcpy(dstRow, srcRow, width);
        }
    }
}

// 8bpp -> 16bpp (RGB565)
void blit_8_to_16(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey)
{
    uint16_t* dstBase = (uint16_t*)dst->data;
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRow = src->data + (srcY + y) * src->pitch + srcX;
        uint16_t* dstRow = (uint16_t*)((uint8_t*)dstBase + (dstY + y) * dst->pitch) + dstX;

        for (int x = 0; x < width; ++x)
        {
            uint8_t index = srcRow[x];
            uint32_t color = src->palette[index]; // ARGB8888

            if (colorKey && color == *colorKey)
                continue;

            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;

            // Pack into RGB565
            uint16_t rgb565 = ((r >> 3) << 11) |
                              ((g >> 2) << 5)  |
                              (b >> 3);

            dstRow[x] = rgb565;
        }
    }
}


// 8bpp -> 24bpp (BGR888)
void blit_8_to_24(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRow = src->data + (srcY + y) * src->pitch + srcX;
        uint8_t* dstRow = dst->data + (dstY + y) * dst->pitch + dstX * 3;

        for (int x = 0; x < width; ++x)
        {
            uint8_t index = srcRow[x];
            uint32_t color = src->palette[index]; // ARGB8888

            if (colorKey && color == *colorKey)
                continue;

            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8) & 0xFF;
            uint8_t b = color & 0xFF;

            dstRow[x * 3 + 0] = b;
            dstRow[x * 3 + 1] = g;
            dstRow[x * 3 + 2] = r;
        }
    }
}


// 8bpp -> 32bpp (ARGB8888)
void blit_8_to_32(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey)
{
    uint32_t* dstBase = (uint32_t*)dst->data;
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRow = src->data + (srcY + y) * src->pitch + srcX;
        uint32_t* dstRow = (uint32_t*)((uint8_t*)dstBase + (dstY + y) * dst->pitch) + dstX;

        for (int x = 0; x < width; ++x)
        {
            uint8_t index = srcRow[x];
            uint32_t color = src->palette[index]; // ARGB8888

            if (colorKey && color == *colorKey)
                continue;

            dstRow[x] = color;
        }
    }
}

/* Helpers */
static inline void rgb565_to_rgb888(uint16_t rgb565, uint8_t *r, uint8_t *g, uint8_t *b)
{
    *r = (uint8_t)((rgb565 >> 11) & 0x1F);
    *g = (uint8_t)((rgb565 >> 5) & 0x3F);
    *b = (uint8_t)(rgb565 & 0x1F);
    /* expand to 8-bit */
    *r = (uint8_t)((*r << 3) | (*r >> 2));
    *g = (uint8_t)((*g << 2) | (*g >> 4));
    *b = (uint8_t)((*b << 3) | (*b >> 2));
}

static inline uint32_t rgb888_to_argb(uint8_t r, uint8_t g, uint8_t b)
{
    return (0xFFu << 24) | (r << 16) | (g << 8) | b;
}

/* Find palette index: exact match first, otherwise nearest (euclidean in RGB space) */
static uint8_t find_palette_index_nearest(uint32_t *palette, uint8_t r, uint8_t g, uint8_t b)
{
    for (int i = 0; i < 256; ++i) {
        uint32_t c = palette[i];
        if ( (uint8_t)(c >> 16) == r && (uint8_t)(c >> 8) == g && (uint8_t)c == b )
            return (uint8_t)i;
    }
    int best = 0;
    unsigned int bestDist = 0xFFFFFFFFu;
    for (int i = 0; i < 256; ++i) {
        uint32_t c = palette[i];
        int pr = (int)((c >> 16) & 0xFF) - (int)r;
        int pg = (int)((c >> 8) & 0xFF) - (int)g;
        int pb = (int)(c & 0xFF) - (int)b;
        unsigned int dist = (unsigned int)(pr*pr + pg*pg + pb*pb);
        if (dist < bestDist) { bestDist = dist; best = i; }
    }
    return (uint8_t)best;
}

/* 16bpp -> 8bpp */
void blit_16_to_8(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey)
{
    if (!dst->palette) return; /* destination needs palette to receive indices */

    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRowBytes = src->data + (srcY + y) * src->pitch + srcX * 2;
        uint8_t* dstRow = dst->data + (dstY + y) * dst->pitch + dstX;

        for (int x = 0; x < width; ++x)
        {
            uint16_t pix = (uint16_t) (srcRowBytes[x*2] | (srcRowBytes[x*2 + 1] << 8));
            uint8_t r,g,b;
            rgb565_to_rgb888(pix, &r, &g, &b);
            uint32_t srcARGB = rgb888_to_argb(r,g,b);

            if (colorKey && srcARGB == *colorKey)
                continue;

            uint8_t idx = find_palette_index_nearest(dst->palette, r, g, b);
            dstRow[x] = idx;
        }
    }
}

/* 16bpp -> 16bpp (copy, honor colorKey if given) */
void blit_16_to_16(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRowBytes = src->data + (srcY + y) * src->pitch + srcX * 2;
        uint8_t* dstRowBytes = dst->data + (dstY + y) * dst->pitch + dstX * 2;

        if (!colorKey) {
            memcpy(dstRowBytes, srcRowBytes, width * 2);
            continue;
        }

        for (int x = 0; x < width; ++x)
        {
            uint16_t pix = (uint16_t)(srcRowBytes[x*2] | (srcRowBytes[x*2 + 1] << 8));
            uint8_t r,g,b;
            rgb565_to_rgb888(pix, &r, &g, &b);
            uint32_t srcARGB = rgb888_to_argb(r,g,b);
            if (srcARGB == *colorKey) continue;

            dstRowBytes[x*2]     = (uint8_t)(pix & 0xFF);
            dstRowBytes[x*2 + 1] = (uint8_t)(pix >> 8);
        }
    }
}

/* 16bpp -> 24bpp (BGR888 destination) */
void blit_16_to_24(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRowBytes = src->data + (srcY + y) * src->pitch + srcX * 2;
        uint8_t* dstRow = dst->data + (dstY + y) * dst->pitch + dstX * 3;

        for (int x = 0; x < width; ++x)
        {
            uint16_t pix = (uint16_t)(srcRowBytes[x*2] | (srcRowBytes[x*2 + 1] << 8));
            uint8_t r,g,b;
            rgb565_to_rgb888(pix, &r, &g, &b);
            uint32_t srcARGB = rgb888_to_argb(r,g,b);
            if (colorKey && srcARGB == *colorKey) continue;

            dstRow[x*3 + 0] = b;
            dstRow[x*3 + 1] = g;
            dstRow[x*3 + 2] = r;
        }
    }
}

/* 16bpp -> 32bpp (ARGB8888 destination, alpha set to 0xFF) */
void blit_16_to_32(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRowBytes = src->data + (srcY + y) * src->pitch + srcX * 2;
        uint32_t* dstRow = (uint32_t*)(dst->data + (dstY + y) * dst->pitch) + dstX;

        for (int x = 0; x < width; ++x)
        {
            uint16_t pix = (uint16_t)(srcRowBytes[x*2] | (srcRowBytes[x*2 + 1] << 8));
            uint8_t r,g,b;
            rgb565_to_rgb888(pix, &r, &g, &b);
            uint32_t srcARGB = rgb888_to_argb(r,g,b);
            if (colorKey && srcARGB == *colorKey) continue;

            dstRow[x] = srcARGB;
        }
    }
}

/* 24bpp = BGR888 source */

/* 24bpp -> 8bpp (palette destination) */
void blit_24_to_8(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey)
{
    if (!dst->palette) return;

    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRow = src->data + (srcY + y) * src->pitch + srcX * 3;
        uint8_t* dstRow = dst->data + (dstY + y) * dst->pitch + dstX;

        for (int x = 0; x < width; ++x)
        {
            uint8_t b = srcRow[x*3 + 0];
            uint8_t g = srcRow[x*3 + 1];
            uint8_t r = srcRow[x*3 + 2];
            uint32_t srcARGB = (0xFFu << 24) | (r << 16) | (g << 8) | b;

            if (colorKey && srcARGB == *colorKey)
                continue;

            uint8_t idx = find_palette_index_nearest(dst->palette, r, g, b);
            dstRow[x] = idx;
        }
    }
}

/* 24bpp -> 16bpp (RGB565 destination) */
void blit_24_to_16(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRow = src->data + (srcY + y) * src->pitch + srcX * 3;
        uint16_t* dstRow = (uint16_t*)(dst->data + (dstY + y) * dst->pitch) + dstX;

        for (int x = 0; x < width; ++x)
        {
            uint8_t b = srcRow[x*3 + 0];
            uint8_t g = srcRow[x*3 + 1];
            uint8_t r = srcRow[x*3 + 2];
            uint32_t srcARGB = (0xFFu << 24) | (r << 16) | (g << 8) | b;

            if (colorKey && srcARGB == *colorKey)
                continue;

            uint16_t rgb565 = ((r >> 3) << 11) |
                              ((g >> 2) << 5)  |
                              (b >> 3);
            dstRow[x] = rgb565;
        }
    }
}

/* 24bpp -> 24bpp (BGR888 copy) */
void blit_24_to_24(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRow = src->data + (srcY + y) * src->pitch + srcX * 3;
        uint8_t* dstRow = dst->data + (dstY + y) * dst->pitch + dstX * 3;

        if (!colorKey) {
            memcpy(dstRow, srcRow, width * 3);
            continue;
        }

        for (int x = 0; x < width; ++x)
        {
            uint8_t b = srcRow[x*3 + 0];
            uint8_t g = srcRow[x*3 + 1];
            uint8_t r = srcRow[x*3 + 2];
            uint32_t srcARGB = (0xFFu << 24) | (r << 16) | (g << 8) | b;

            if (srcARGB == *colorKey)
                continue;

            dstRow[x*3 + 0] = b;
            dstRow[x*3 + 1] = g;
            dstRow[x*3 + 2] = r;
        }
    }
}

/* 24bpp -> 32bpp (ARGB8888 destination, alpha = 0xFF) */
void blit_24_to_32(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint8_t* srcRow = src->data + (srcY + y) * src->pitch + srcX * 3;
        uint32_t* dstRow = (uint32_t*)(dst->data + (dstY + y) * dst->pitch) + dstX;

        for (int x = 0; x < width; ++x)
        {
            uint8_t b = srcRow[x*3 + 0];
            uint8_t g = srcRow[x*3 + 1];
            uint8_t r = srcRow[x*3 + 2];
            uint32_t srcARGB = (0xFFu << 24) | (r << 16) | (g << 8) | b;

            if (colorKey && srcARGB == *colorKey)
                continue;

            dstRow[x] = srcARGB;
        }
    }
}

/* 32bpp = ARGB8888 source */

/* 32bpp -> 8bpp (palette destination) */
void blit_32_to_8(Surface* dst, int dstX, int dstY,
                  Surface* src, int srcX, int srcY,
                  int width, int height, uint32_t* colorKey)
{
    if (!dst->palette) return;

    for (int y = 0; y < height; ++y)
    {
        uint32_t* srcRow = (uint32_t*)(src->data + (srcY + y) * src->pitch) + srcX;
        uint8_t* dstRow = dst->data + (dstY + y) * dst->pitch + dstX;

        for (int x = 0; x < width; ++x)
        {
            uint32_t color = srcRow[x]; // ARGB8888
            if (colorKey && color == *colorKey)
                continue;

            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8)  & 0xFF;
            uint8_t b = color & 0xFF;

            uint8_t idx = find_palette_index_nearest(dst->palette, r, g, b);
            dstRow[x] = idx;
        }
    }
}

/* 32bpp -> 16bpp (RGB565 destination) */
void blit_32_to_16(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint32_t* srcRow = (uint32_t*)(src->data + (srcY + y) * src->pitch) + srcX;
        uint16_t* dstRow = (uint16_t*)(dst->data + (dstY + y) * dst->pitch) + dstX;

        for (int x = 0; x < width; ++x)
        {
            uint32_t color = srcRow[x]; // ARGB8888
            if (colorKey && color == *colorKey)
                continue;

            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8)  & 0xFF;
            uint8_t b = color & 0xFF;

            uint16_t rgb565 = ((r >> 3) << 11) |
                              ((g >> 2) << 5)  |
                              (b >> 3);
            dstRow[x] = rgb565;
        }
    }
}

/* 32bpp -> 24bpp (BGR888 destination) */
void blit_32_to_24(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint32_t* srcRow = (uint32_t*)(src->data + (srcY + y) * src->pitch) + srcX;
        uint8_t* dstRow = dst->data + (dstY + y) * dst->pitch + dstX * 3;

        for (int x = 0; x < width; ++x)
        {
            uint32_t color = srcRow[x]; // ARGB8888
            if (colorKey && color == *colorKey)
                continue;

            uint8_t r = (color >> 16) & 0xFF;
            uint8_t g = (color >> 8)  & 0xFF;
            uint8_t b = color & 0xFF;

            dstRow[x*3 + 0] = b;
            dstRow[x*3 + 1] = g;
            dstRow[x*3 + 2] = r;
        }
    }
}

/* 32bpp -> 32bpp (ARGB8888 copy) */
void blit_32_to_32(Surface* dst, int dstX, int dstY,
                   Surface* src, int srcX, int srcY,
                   int width, int height, uint32_t* colorKey)
{
    for (int y = 0; y < height; ++y)
    {
        uint32_t* srcRow = (uint32_t*)(src->data + (srcY + y) * src->pitch) + srcX;
        uint32_t* dstRow = (uint32_t*)(dst->data + (dstY + y) * dst->pitch) + dstX;

        if (!colorKey) {
            memcpy(dstRow, srcRow, width * 4);
            continue;
        }

        for (int x = 0; x < width; ++x)
        {
            uint32_t color = srcRow[x];
            if (color == *colorKey)
                continue;
            dstRow[x] = color;
        }
    }
}




