#include "game.hpp"

#include <zorro/stb_sprintf.h>
#include <cmath>
#include <windows.h>

zorro::GameParams Game::getParams() const
{
    return zorro::GameParams
    {
        "Flappy",
        { 320, 240 }
    };
}

bool Game::onInit(zorro::IEngine& engine)
{
    auto palette = engine.loadPalette("doge.pal");
    engine.setPalette(palette);

    _tiles1._bitmap = engine.loadBitmap("tiles1.bmp");
    _tiles1._imageCount = 3;
    _tiles1._width = _tiles1._bitmap->width() / _tiles1._imageCount;
    _tiles1._height = _tiles1._bitmap->height();
    _tiles1._colorKey = 195;

    _accel = 100.0f;
    _vel = 0.0f;
    _pos.x = (320 - (_tiles1._width)) / 2.0f;
    _pos.y = (240 - (_tiles1._height)) / 2.0f;
    return true;
}

bool Game::onUpdate(zorro::IEngine& engine, double dT)
{
    if (engine.getKeyState(zorro::KeyID::Escape).down)
    {
        engine.quit();
        return true;
    }
    
    if (engine.getKeyState(zorro::KeyID::Space).down)
    {
        if (_vel > 10.0f)
        {
            _vel = -110.0f;
        }
    }

    _vel = _vel + (_accel * dT);
    _pos.y = _pos.y + (_vel * dT);


    char buffer[64];
    stbsp_snprintf(buffer, sizeof(buffer), "a=%0.2f v=%0.2f x=%0.2f y=%0.2f", _accel, _vel, _pos.x, _pos.y);
    engine.setDebugText(buffer);
    return true;
}

bool Game::onRender(zorro::IEngine& engine, double lag)
{
    engine.clearScreen(128);

    int image;
    if (_vel > 5.0f)
    {
        image = 2;
    }
    else if (_vel > -5.0f)
    {
        image = 1;
    }
    else
    {
        image = 0;
    }
    _tiles1.blt(std::round(_pos.x), std::round(_pos.y), image);
    return true;
}

void Game::onCleanup(zorro::IEngine& engine)
{
}

zorro::IGame* zorro::makeGame()
{
    return new Game;
}

void SpriteSheet::blt(int dstX, int dstY, int imageIndex)
{
    _bitmap->blt(dstX, dstY, imageIndex * _width, 0, _width, _height, _colorKey);
}

