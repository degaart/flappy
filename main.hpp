#pragma once

#include <SDL3/SDL.h>
#include <vector>

struct Bitmap
{
    int w, h;
    const uint8_t* data;
};

class Main
{
public:
    SDL_AppResult onInit(int argc, char* argv[]);
    SDL_AppResult onEvent(SDL_Event* event);
    SDL_AppResult onIterate();
    void onQuit(SDL_AppResult result);
private:
    SDL_Window* _window;
    SDL_Renderer* _renderer;
    SDL_Surface* _backbuffer;
    SDL_Texture* _backbufferTexture;
    Bitmap _background;
    std::vector<uint8_t> _palette;
    std::vector<uint32_t> _palette32;

    int _frames;
    uint64_t _fpsTimer;
    uint64_t _prevTime;
    int _fps;
};


