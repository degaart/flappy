#include "engine.hpp"

#include "game.hpp"
#include "util.hpp"
#include <optional>
#include <stdio.h>
#include <stdlib.h>
#include <string>

void Engine::setPalette(const std::vector<glm::vec3>& palette)
{
    if (palette.size() != 256)
    {
        fprintf(stderr, "Invalid palette: incorrect number of colors\n");
        abort();
    }

    /*
     * reserved indices:
     * 0: black
     * 255: white
     * 1 - 9: reserved
     * 246 - 254: reserved
     */
    _palette->colors[0].r = 0;
    _palette->colors[0].g = 0;
    _palette->colors[0].b = 0;

    _palette->colors[255].r = 255;
    _palette->colors[255].g = 255;
    _palette->colors[255].b = 255;

    for (int i = 1; i < 254; i++)
    {
        _palette->colors[i].r = std::round(palette[i].r * 255.0f);
        _palette->colors[i].g = std::round(palette[i].g * 255.0f);
        _palette->colors[i].b = std::round(palette[i].b * 255.0f);
    }
}

Bitmap Engine::loadBitmap(const char* filename, int w, int h)
{
    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        fprintf(stderr, "Failed to load file %s\n", filename);
        abort();
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    Bitmap result;
    result.w = w;
    result.h = h;
    result.data.reserve(size);

    if (fread(result.data.data(), 1, size, f) != size)
    {
        fclose(f);
        fprintf(stderr, "Failed to read file %s\n", filename);
        abort();
    }
    fclose(f);

    return result;
}

std::vector<glm::vec3> Engine::loadPalette(const char* filename)
{
    FILE* f = fopen(filename, "rt");
    if (!f)
    {
        fprintf(stderr, "Failed to load file %s\n", filename);
        abort();
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
        abort();
    }
    else if (auto version = getLine(); !version || *version != "0100\r\n")
    {
        fprintf(stderr, "Invalid header (version) for %s\n", filename);
        abort();
    }
    else if (auto version = getLine(); !version || *version != "256\r\n")
    {
        fprintf(stderr, "Invalid header (colorcount) for %s\n", filename);
        abort();
    }

    std::vector<glm::vec3> result;
    result.reserve(256);
    for (int i = 0; i < 256; i++)
    {
        auto line = getLine();
        if (!line)
        {
            fprintf(stderr, "Failed to read entry %d in %s\n", i, filename);
            abort();
        }

        while (!line->empty() && (line->back() == '\r' || line->back() == '\n'))
        {
            line->pop_back();
        }

        auto tokens = split(*line, " ");
        if (tokens.size() != 3)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s\n", line->c_str(), filename);
            abort();
        }

        auto r = parseInt(tokens[0]);
        if (!r)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s\n", line->c_str(), filename);
            abort();
        }

        auto g = parseInt(tokens[1]);
        if (!g)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s\n", line->c_str(), filename);
            abort();
        }

        auto b = parseInt(tokens[2]);
        if (!b)
        {
            fprintf(stderr, "Invalid entry format \"%s\" in %s\n", line->c_str(), filename);
            abort();
        }

        result.emplace_back(*r / 255.0f, *g / 255.0f, *b / 255.0f);
    }

    return result;
}

SDL_AppResult Engine::onInit()
{
    _frames = 0;
    _fpsTimer = 0;
    _prevTime = 0;
    _fps = 0;

    auto size = Game::getGameScreenSize();
    if (!SDL_CreateWindowAndRenderer(Game::getName(), size.x, size.y, SDL_WINDOW_RESIZABLE, &_window, &_renderer))
    {
        fprintf(stderr, "Failed to create window\n");
        return SDL_APP_FAILURE;
    }

    _backbuffer = SDL_CreateSurface(size.x, size.y, SDL_PIXELFORMAT_INDEX8);
    if (!_backbuffer)
    {
        fprintf(stderr, "Failed to create backbuffer\n");
        return SDL_APP_FAILURE;
    }

    _palette = SDL_CreateSurfacePalette(_backbuffer);
    if (!_palette)
    {
        fprintf(stderr, "Failed to create backbuffer palette\n");
        return SDL_APP_FAILURE;
    }
    if (_palette->ncolors != 256)
    {
        fprintf(stderr, "Invalid number of colors in palette\n");
        return SDL_APP_FAILURE;
    }

#if 0
    /*
     * reserved indices:
     * 0: black
     * 255: white
     * 1 - 9: reserved
     * 246 - 254: reserved
     */
    auto palette = _game->getPalette();
    if (palette.size() != 256)
    {
        fprintf(stderr, "Invalid palette: invalid number of colors: %zu\n", palette.size());
        return SDL_APP_FAILURE;
    }
    else if (palette[0].r != 0.0f || palette[0].g != 0.0f || palette[0].b != 0.0f || palette[255].r != 1.0f || palette[255].g != 1.0f || palette[255].b != 1.0f)
    {
        fprintf(stderr, "Invalid palette: reserved colors were not respected\n");
        return SDL_APP_FAILURE;
    }

    for (int i = 0; i < 255; i++)
    {
        pal->colors[i].r = std::round(palette[i].r * 255.0f);
        pal->colors[i].g = std::round(palette[i].g * 255.0f);
        pal->colors[i].b = std::round(palette[i].b * 255.0f);
    }
#endif

    // auto image = loadBitmap("doge.raw", 640, 480);
    // if (!image)
    //{
    //     return SDL_APP_FAILURE;
    // }
    //_background = std::move(*image);

    _game = new Game;
    if (!_game->onInit(*this))
    {
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult Engine::onEvent(SDL_Event* event)
{
    switch (event->type)
    {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;
    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_KEY_DOWN:
        if (event->key.key == SDLK_LEFT)
        {
            _keyState.left = event->type == SDL_EVENT_KEY_DOWN;
        }
        else if (event->key.key == SDLK_RIGHT)
        {
            _keyState.right = event->type == SDL_EVENT_KEY_DOWN;
        }
        else if (event->key.key == SDLK_UP)
        {
            _keyState.up = event->type == SDL_EVENT_KEY_DOWN;
        }
        else if (event->key.key == SDLK_DOWN)
        {
            _keyState.down = event->type == SDL_EVENT_KEY_DOWN;
        }
        else if (event->key.key == SDLK_ESCAPE)
        {
            _keyState.escape = event->type == SDL_EVENT_KEY_DOWN;
        }
        break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult Engine::onIterate()
{
    SDL_SetRenderDrawColor(_renderer, 0, 0, 0, 0xFF);
    SDL_RenderClear(_renderer);

    SDL_LockSurface(_backbuffer);
    if (!_game->onRender(*this))
    {
        fprintf(stderr, "Rendering failed\n");
        abort();
    }
    SDL_UnlockSurface(_backbuffer);

    SDL_Texture* tex = SDL_CreateTextureFromSurface(_renderer, _backbuffer);
    SDL_RenderTexture(_renderer, tex, nullptr, nullptr);
    SDL_DestroyTexture(tex);

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
    snprintf(debugText, sizeof(debugText), "fps=%d", _fps);
    SDL_SetRenderClipRect(_renderer, nullptr);
    SDL_SetRenderDrawColor(_renderer, 0x7F, 0x00, 0xFF, 0xFF);
    SDL_RenderDebugText(_renderer, 10.0f, 10.0f, debugText);

    SDL_RenderPresent(_renderer);
    return SDL_APP_CONTINUE;
}

void Engine::onQuit(SDL_AppResult result)
{
    SDL_DestroySurface(_backbuffer);
    delete _game;
}

const Keystate& Engine::keyState() const
{
    return _keyState;
}

void Engine::blit(const Bitmap& bmp, int x, int y, int w, int h)
{
    const uint8_t* srcPtr = bmp.data.data();
    uint8_t* dstPtr = (uint8_t*)_backbuffer->pixels;
    assert(_backbuffer->format == SDL_PIXELFORMAT_INDEX8);
    for (int j = 0; j < _backbuffer->h; j++)
    {
        memcpy(dstPtr, srcPtr, bmp.w);
        dstPtr += _backbuffer->pitch;
        srcPtr += bmp.w;
    }
}

