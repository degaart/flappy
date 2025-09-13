#pragma once

#include <SDL3/SDL.h>

struct Bitmap
{
    int w, h;
    uint8_t* data;
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
};


