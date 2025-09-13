#include "engine.hpp"

#include "game.hpp"
#include "util.hpp"
#include "stb_image.h"
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

Bitmap Engine::loadBitmap(const char* filename)
{
    int channels;
    int w, h;
    auto imageData = stbi_load(filename, &w, &h, &channels, 0);
    if (!imageData)
    {
        fprintf(stderr, "Failed to load file %s\n", filename);
        abort();
    }

    /*
     * imageData is in RGB format
     * To convert it into palettized format, we use the euclidian distance
     * in a 3D space to find the nearest color
     */
    Bitmap result;
    result.w = w;
    result.h = h;
    result.data.reserve(w * h);
    uint8_t* srcPtr = imageData;
    for (int j = 0; j < h; j++)
    {
        for (int i = 0; i < w; i++)
        {
            uint8_t r = *srcPtr++;
            uint8_t g = *srcPtr++;
            uint8_t b = *srcPtr++;

            int closest = 0;
            float minDist = FLT_MAX;
            for (int col = 0; col < 256; col++)
            {
                float rDist = r - _palette->colors[col].r;
                float gDist = g - _palette->colors[col].g;
                float bDist = b - _palette->colors[col].b;
                float dist = rDist*rDist + gDist*gDist + bDist*bDist;
                if (dist < minDist)
                {
                    minDist = dist;
                    closest = col;
                }
            }
            result.data.push_back(closest);
        }
    }
    stbi_image_free(imageData);
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
    _fpsTimer = 0.0;
    _prevTime = 0.0;
    _lag = 0.0;
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
    _debugText.clear();

    auto beginTime = getTime();
    auto elapsed = beginTime - _prevTime;
    _lag += elapsed;
    while (_lag > dT)
    {
        _game->onUpdate(*this, dT);
        _lag -= dT;
    }

    setDrawColor({0.39f, 0.58f, 0.93f});
    SDL_RenderClear(_renderer);

    SDL_LockSurface(_backbuffer);
    memset(_backbuffer->pixels, 0, _backbuffer->pitch * _backbuffer->h);
    if (!_game->onRender(*this, _lag / dT))
    {
        fprintf(stderr, "Rendering failed\n");
        abort();
    }
    SDL_UnlockSurface(_backbuffer);

    int gameWidth, gameHeight;
    gameWidth = _backbuffer->w;
    gameHeight = _backbuffer->h;

    int windowWidth, windowHeight;
    SDL_GetRenderOutputSize(_renderer, &windowWidth, &windowHeight);

    SDL_FRect dstRc;
    dstRc.w = gameWidth;
    dstRc.h = gameHeight;

    if (((float)windowWidth / windowHeight) > ((float)gameWidth / gameHeight))
    {
        dstRc.h = windowHeight;
        dstRc.w = (dstRc.h * gameWidth) / gameHeight;
    }
    else
    {
        dstRc.w = windowWidth;
        dstRc.h = (dstRc.w * gameHeight) / gameWidth;
    }

    dstRc.x = (windowWidth - dstRc.w) / 2.0f;
    dstRc.y = (windowHeight - dstRc.h) / 2.0f;

    SDL_Texture* tex = SDL_CreateTextureFromSurface(_renderer, _backbuffer);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_NEAREST);
    SDL_RenderTexture(_renderer, tex, nullptr, &dstRc);
    SDL_DestroyTexture(tex);

    _frames++;
    _fpsTimer += elapsed;
    if (_fpsTimer >= 1.0)
    {
        _fps = _frames / _fpsTimer;
        _fpsTimer = 0;
        _frames = 0;
    }
    _prevTime = beginTime;

    char debugText[512];
    snprintf(debugText, sizeof(debugText), "fps=%d %s", _fps, _debugText.c_str());
    SDL_SetRenderClipRect(_renderer, nullptr);
    SDL_SetRenderDrawColor(_renderer, 0x7F, 0x7F, 0x7F, 0xFF);
    setDrawColor({1.0f, 1.0f, 0.25f});
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

static void transparentBlit8(
    const uint8_t* srcPixels, int srcWidth, int srcHeight, int srcPitch,
    uint8_t* dstPixels, int dstWidth, int dstHeight, int dstPitch,
    int srcX, int srcY, int blitWidth, int blitHeight, int dstX, int dstY,
    uint8_t colorKey
)
{
    if (blitWidth <= 0 || blitHeight <= 0)
    {
        return;
    }

    // Clip source rect
    if (srcX < 0)
    {
        blitWidth += srcX;
        dstX -= srcX;
        srcX = 0;
    }
    if (srcY < 0)
    {
        blitHeight += srcY;
        dstY -= srcY;
        srcY = 0;
    }
    if (srcX + blitWidth > srcWidth)
    {
        blitWidth = srcWidth - srcX;
    }
    if (srcY + blitHeight > srcHeight)
    {
        blitHeight = srcHeight - srcY;
    }

    // Clip dest rect
    if (dstX < 0)
    {
        blitWidth += dstX;
        srcX -= dstX;
        dstX = 0;
    }
    if (dstY < 0)
    {
        blitHeight += dstY;
        srcY -= dstY;
        dstY = 0;
    }
    if (dstX + blitWidth > dstWidth)
    {
        blitWidth = dstWidth - dstX;
    }
    if (dstY + blitHeight > dstHeight)
    {
        blitHeight = dstHeight - dstY;
    }

    if (blitWidth <= 0 || blitHeight <= 0)
    {
        return;
    }

    const uint8_t* srcRow = srcPixels + srcY * srcPitch + srcX;
    uint8_t* dstRow = dstPixels + dstY * dstPitch + dstX;
    for (int y = 0; y < blitHeight; ++y)
    {
        auto srcPtr = srcRow;
        auto dstPtr = dstRow;
        for (int x = 0; x < blitWidth; x++)
        {
            if (*srcPtr != colorKey)
            {
                *dstPtr = *srcPtr;
            }
            srcPtr++;
            dstPtr++;
        }
        srcRow += srcPitch;
        dstRow += dstPitch;
    }
}

static void blit8(const uint8_t* srcPixels, int srcWidth, int srcHeight, int srcPitch, uint8_t* dstPixels, int dstWidth, int dstHeight, int dstPitch, int srcX,
                  int srcY, int blitWidth, int blitHeight, int dstX, int dstY)
{
    if (blitWidth <= 0 || blitHeight <= 0)
    {
        return;
    }

    // Clip source rect
    if (srcX < 0)
    {
        blitWidth += srcX;
        dstX -= srcX;
        srcX = 0;
    }
    if (srcY < 0)
    {
        blitHeight += srcY;
        dstY -= srcY;
        srcY = 0;
    }
    if (srcX + blitWidth > srcWidth)
    {
        blitWidth = srcWidth - srcX;
    }
    if (srcY + blitHeight > srcHeight)
    {
        blitHeight = srcHeight - srcY;
    }

    // Clip dest rect
    if (dstX < 0)
    {
        blitWidth += dstX;
        srcX -= dstX;
        dstX = 0;
    }
    if (dstY < 0)
    {
        blitHeight += dstY;
        srcY -= dstY;
        dstY = 0;
    }
    if (dstX + blitWidth > dstWidth)
    {
        blitWidth = dstWidth - dstX;
    }
    if (dstY + blitHeight > dstHeight)
    {
        blitHeight = dstHeight - dstY;
    }

    if (blitWidth <= 0 || blitHeight <= 0)
    {
        return;
    }

    const uint8_t* srcRow = srcPixels + srcY * srcPitch + srcX;
    uint8_t* dstRow = dstPixels + dstY * dstPitch + dstX;

    for (int y = 0; y < blitHeight; ++y)
    {
        memcpy(dstRow, srcRow, blitWidth);
        srcRow += srcPitch;
        dstRow += dstPitch;
    }
}

void Engine::blit(const Bitmap& bmp, int srcX, int srcY, int srcW, int srcH, int dstX, int dstY)
{
    blit8(bmp.data.data(), bmp.w, bmp.h, bmp.w, (uint8_t*)_backbuffer->pixels, _backbuffer->w, _backbuffer->h, _backbuffer->pitch, srcX, srcY, srcW, srcH, dstX,
          dstY);
}

void Engine::blit(const Bitmap& bmp,
          int srcX, int srcY, int srcW, int srcH,
          int dstX, int dstY,
          uint8_t colorKey)
{
    transparentBlit8(bmp.data.data(), bmp.w, bmp.h, bmp.w, (uint8_t*)_backbuffer->pixels, _backbuffer->w, _backbuffer->h, _backbuffer->pitch, srcX, srcY, srcW, srcH, dstX,
          dstY, colorKey);
}

void Engine::setDrawColor(glm::vec3 color)
{
    SDL_SetRenderDrawColor(_renderer, std::round(color.r * 255.0f), std::round(color.g * 255.0f), std::round(color.b * 255.0f), 0xFF);
}

double Engine::getTime() const
{
    return SDL_GetTicks() / 1000.0;
}

void Engine::setDebugText(const char* text)
{
    if (!_debugText.empty())
    {
        _debugText += " ";
    }
    _debugText += text;
}

