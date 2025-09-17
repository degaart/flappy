#pragma once

#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>

struct Keystate
{
    bool left;
    bool right;
    bool up;
    bool down;
    bool space;
    bool escape;
};

class Bitmap
{
public:
    int w, h;
    std::vector<uint8_t> data;
};

class Game;

class Engine
{
public:
    static constexpr float dT = 1.0f / 60.0f;

    void setPalette(const std::vector<glm::vec3>& palette);
    Bitmap loadBitmap(const char* filename);
    static std::vector<glm::vec3> loadPalette(const char* filename);
    const Keystate& keyState() const;
    void blit(const Bitmap& bmp,
              int srcX, int srcY, int srcW, int srcH,
              int dstX, int dstY);
    void blit(const Bitmap& bmp,
              int srcX, int srcY, int srcW, int srcH,
              int dstX, int dstY,
              uint8_t colorKey);
    double getTime() const;
    void clear();
    void setDebugText(const char* text);

private:
};

void __panic(const char* file, int line,
             const char* format, ...);
#define panic(...) __panic(__FILE__,__LINE__, __VA_ARGS__)



