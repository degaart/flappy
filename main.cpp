#include "main.hpp"

#include "util.hpp"

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <assert.h>
#include <optional>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string_view>
#include <vector>

extern "C" const char* __asan_default_options()
{
    return "detect_leaks=0";
}

std::optional<Bitmap> loadBitmap(const char* filename, int w, int h)
{
    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to load file %s\n", filename);
        return std::nullopt;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    auto buffer = static_cast<uint8_t*>(malloc(size));
    if (fread(buffer, 1, size, f) != size)
    {
        fclose(f);
        free(buffer);
        fprintf(stderr, "Failed to read file %s\n", filename);
        return std::nullopt;
    }
    fclose(f);

    Bitmap result;
    result.w = w;
    result.h = h;
    result.data = buffer;

    return result;
}

/* Returns a dynamically-allocated array of 3*256 uint8_t values */
std::optional<std::vector<uint8_t>> loadPalette(const char* filename)
{
    FILE* f = fopen(filename, "rt");
    if (!f)
    {
        fprintf(stderr, "Failed to load file %s\n", filename);
        return std::nullopt;
    }

    auto getLine = [f]() -> std::optional<std::string>
    {
        char buffer[512];
        if (!fgets(buffer, sizeof(buffer), f))
        {
            return std::nullopt;
        }

        return buffer;
    };

    if (auto header = getLine(); !header || *header != "JASC-PAL\r\n")
    {
        fprintf(stderr, "Invalid header (magic) for %s\n", filename);
        return std::nullopt;
    }
    else if (auto version = getLine(); !version || *version != "0100\r\n")
    {
        fprintf(stderr, "Invalid header (version) for %s\n", filename);
        return std::nullopt;
    }
    else if (auto version = getLine(); !version || *version != "256\r\n")
    {
        fprintf(stderr, "Invalid header (colorcount) for %s\n", filename);
        return std::nullopt;
    }

    std::vector<uint8_t> result;
    result.reserve(256 * 3);
    for (int i = 0; i < 256; i++)
    {
        auto line = getLine();
        if (!line)
        {
            fprintf(stderr, "Failed to read entry %d in %s\n", i, filename);
            return std::nullopt;
        }

        while (!line->empty() && (line->back() == '\r' || line->back() == '\n'))
        {
            line->pop_back();
        }

        auto tokens = split(*line, " ");
        if (tokens.size() != 3)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s\n", line->c_str(), filename);
            return std::nullopt;
        }

        auto r = parseInt(tokens[0]);
        if (!r)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s\n", line->c_str(), filename);
            return std::nullopt;
        }
        result.push_back(*r);

        auto g = parseInt(tokens[1]);
        if (!g)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s\n", line->c_str(), filename);
            return std::nullopt;
        }
        result.push_back(*g);

        auto b = parseInt(tokens[2]);
        if (!b)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s\n", line->c_str(), filename);
            return std::nullopt;
        }
        result.push_back(*b);
    }

    return result;
}

SDL_AppResult Main::onInit(int argc, char* argv[])
{
    _frames = 0;
    _fpsTimer = 0;
    _prevTime = 0;
    _fps = 0;
    _renderMethod = 0;

    if (!SDL_CreateWindowAndRenderer("Flappy", 640, 480, SDL_WINDOW_RESIZABLE, &_window, &_renderer))
    {
        fprintf(stderr, "Failed to create window\n");
        return SDL_APP_FAILURE;
    }

    int width, height;
    SDL_GetWindowSizeInPixels(_window, &width, &height);

    _backbuffer = SDL_CreateSurface(640, 480, SDL_PIXELFORMAT_INDEX8);
    if (!_backbuffer)
    {
        fprintf(stderr, "Failed to create backbuffer\n");
        return SDL_APP_FAILURE;
    }

    SDL_Palette* pal = SDL_CreateSurfacePalette(_backbuffer);
    if (!pal)
    {
        fprintf(stderr, "Failed to create backbuffer palette\n");
        return SDL_APP_FAILURE;
    }

    auto palette = loadPalette("doge.pal");
    if (!palette)
    {
        return SDL_APP_FAILURE;
    }
    _palette = *palette;

    _palette32.reserve(255);
    for (int i = 0; i < 255; i++)
    {
        uint8_t r = _palette[i * 3];
        uint8_t g = _palette[(i*3)+1];
        uint8_t b = _palette[(i*3)+2];
        _palette32.push_back(b | (g << 8) | (r << 16) | (255 << 24));
    }
    _backbufferTexture = SDL_CreateTexture(_renderer, SDL_PIXELFORMAT_BGRX32, SDL_TEXTUREACCESS_STREAMING, 640, 480);
    if (!_backbufferTexture)
    {
        fprintf(stderr, "Failed to create texture: %s\n", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    /*
     * reserved indices:
     * 0: black
     * 255: white
     * 1 - 9: reserved
     * 246 - 254: reserved
     */
    for (int i = 0; i < 256; i++)
    {
        pal->colors[i].r = (*palette)[i * 3];
        pal->colors[i].g = (*palette)[(i * 3) + 1];
        pal->colors[i].b = (*palette)[(i * 3) + 2];
        pal->colors[i].a = 0;
    }

    auto image = loadBitmap("doge.raw", 640, 480);
    if (!image)
    {
        return SDL_APP_FAILURE;
    }
    _background = std::move(*image);
    return SDL_APP_CONTINUE;
}

SDL_AppResult Main::onEvent(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_KEY_UP:
        if (event->key.key == SDLK_ESCAPE)
        {
            return SDL_APP_SUCCESS;
        }
        else if (event->key.key == SDLK_SPACE)
        {
            _renderMethod = !_renderMethod;
        }
        break;
    }
    return SDL_APP_CONTINUE;
}

void Main::render1()
{
    SDL_LockSurface(_backbuffer);

    if (_backbuffer->pitch == _background.w)
    {
        memcpy(_backbuffer->pixels, _background.data, _background.w * _background.h);
    }
    else
    {

        uint8_t* srcPtr = (uint8_t*)_background.data;
        uint8_t* dstPtr = (uint8_t*)_backbuffer->pixels;
        assert(_backbuffer->format == SDL_PIXELFORMAT_INDEX8);
        for (int j = 0; j < _backbuffer->h; j++)
        {
            uint8_t* line = dstPtr;
            for (int i = 0; i < _backbuffer->w; i++)
            {
                *line = *srcPtr;
                srcPtr++;
                line++;
            }
            dstPtr += _backbuffer->pitch;
        }
    }
    SDL_UnlockSurface(_backbuffer);

    SDL_Texture* tex = SDL_CreateTextureFromSurface(_renderer, _backbuffer);
    SDL_RenderTexture(_renderer, tex, nullptr, nullptr);
    SDL_DestroyTexture(tex);
}

void Main::render2()
{
    SDL_Surface* surf;
    if (!SDL_LockTextureToSurface(_backbufferTexture, nullptr, &surf))
    {
        abort();
    }
    assert(surf->format == SDL_PIXELFORMAT_BGRX32);

    const uint8_t* srcPtr = _background.data;
    uint8_t* dstPtr = static_cast<uint8_t*>(surf->pixels);
    for (int j = 0; j < surf->h; j++)
    {
        uint32_t* line = reinterpret_cast<uint32_t*>(dstPtr);
        for (int i = 0; i < surf->w; i++)
        {
            int c = *srcPtr;
            *line = _palette32[c];
            line++;
            srcPtr++;
        }
        dstPtr += surf->pitch;
    }

    SDL_UnlockTexture(_backbufferTexture);
    SDL_RenderTexture(_renderer, _backbufferTexture, nullptr, nullptr);
}

SDL_AppResult Main::onIterate()
{
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 0xFF);
    SDL_RenderClear(_renderer);

    if (_renderMethod == 0)
    {
        render1();
    }
    else
    {
        render2();
    }

    uint64_t now = SDL_GetTicks();
    _frames++;
    _fpsTimer += now - _prevTime;
    if (_fpsTimer >= 1000)
    {
        _fps = (_frames * 1000) / _fpsTimer;
        _fpsTimer = 0;
        _frames = 0;
    }
    _prevTime = now;

    char debugText[512];
    snprintf(debugText, sizeof(debugText), "fps=%d rendermethod=%d", _fps, _renderMethod);
    SDL_SetRenderClipRect(_renderer, nullptr);
    SDL_SetRenderDrawColor(_renderer, 0x7F, 0x00, 0xFF, 0xFF);
    SDL_RenderDebugText(_renderer, 10.0f, 10.0f, debugText);

    SDL_RenderPresent(_renderer);
    return SDL_APP_CONTINUE;
}

void Main::onQuit(SDL_AppResult result)
{
}

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[])
{
    Main* main = new Main;
    *appstate = main;
    return main->onInit(argc, argv);
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    Main* main = static_cast<Main*>(appstate);
    return main->onEvent(event);
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    Main* main = static_cast<Main*>(appstate);
    return main->onIterate();
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    Main* main = static_cast<Main*>(appstate);
    main->onQuit(result);
    delete main;
}
