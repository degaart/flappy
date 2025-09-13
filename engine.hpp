#pragma once

#include <stdint.h>
#include <vector>
#include <glm/glm.hpp>
#include <SDL3/SDL.h>
#include <string>

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

#ifdef MAIN_CPP
#define private public
#endif

class Engine
{
public:
    static constexpr float dT = 1.0f / 60.0f;

    void setPalette(const std::vector<glm::vec3>& palette);
    Bitmap loadBitmap(const char* filename, int w, int h);
    static std::vector<glm::vec3> loadPalette(const char* filename);
    const Keystate& keyState() const;
    void blit(const Bitmap& bmp,
              int srcX, int srcY, int srcW, int srcH,
              int dstX, int dstY);
    double getTime() const;
    void clear();
    void setDebugText(const char* text);

private:
    SDL_Window* _window;
    SDL_Renderer* _renderer;
    SDL_Surface* _backbuffer;
    Game* _game;
    SDL_Palette* _palette;
    int _frames;
    double _fpsTimer;
    double _prevTime;
    double _lag;
    int _fps;
    Keystate _keyState;
    std::string _debugText;

    SDL_AppResult onInit();
    SDL_AppResult onEvent(SDL_Event* event);
    SDL_AppResult onIterate();
    void onQuit(SDL_AppResult result);

    void setDrawColor(glm::vec3 color);
};

#ifdef MAIN_CPP
#undef private
#endif
