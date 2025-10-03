#pragma once

#include <stdint.h>

namespace zorro
{
    template <typename T>
    struct Size
    {
        T width;
        T height;
    };

    template <typename T>
    struct Point
    {
        T x;
        T y;
    };

    template <typename T>
    struct Rect
    {
        T x, y;
        T w, h;
    };

    template <typename T>
    struct Color
    {
        T r;
        T g;
        T b;
    };

    struct IBitmap
    {
        virtual ~IBitmap() = default;
        virtual int width() const = 0;
        virtual int height() const = 0;
        virtual void blt(int dstX, int dstY,
                         int srcX, int srcY, int srcWidth, int srcHeight,
                         int colorKey = -1) = 0;
    };

    struct ISfx
    {
        virtual ~ISfx() = default;
        virtual void setFreq(int freq) = 0;
        virtual void play() = 0;
        virtual void stop() = 0;
    };

    struct IPalette
    {
        virtual ~IPalette() = default;
        virtual int colorCount() const = 0;
        virtual Color<uint8_t> colorAt(int index) const = 0;
    };

    enum class KeyID
    {
        Left,
        Right,
        Up,
        Down,
        Space,
        Escape
    };

    struct KeyState
    {
        bool down;
        bool repeat;
    };

    struct IEngine
    {
        virtual ~IEngine() = default;
        /* Lifetime is managed by engine */
        virtual IBitmap* loadBitmap(const char* filename) = 0;
        /* Lifetime is managed by engine */
        virtual ISfx* loadSfx(const char* filename) = 0;
        /* Lifetime is managed by engine */
        virtual IPalette* loadPalette(const char* filename) = 0;
        virtual void clearScreen(uint8_t color) = 0;
        virtual void setDebugText(const char* debugText) = 0;
        virtual KeyState getKeyState(KeyID key) const = 0;
        virtual void setPalette(const IPalette* palette) = 0;
        virtual double getTime() const = 0;
        virtual void quit() = 0;
    };
}

