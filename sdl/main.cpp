#define MAIN_CPP
#include "engine.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <assert.h>
#include <stdlib.h>

#if 0
extern "C" const char* __asan_default_options()
{
    return "detect_leaks=0";
}
#endif

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    Engine* engine = new Engine;
    *appstate = engine;
    return engine->onInit();
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    auto engine = static_cast<Engine*>(appstate);
    return engine->onEvent(event);
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    auto engine = static_cast<Engine*>(appstate);
    return engine->onIterate();
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    auto engine = static_cast<Engine*>(appstate);
    engine->onQuit(result);
    delete engine;
}

