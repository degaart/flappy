#include "game.hpp"

#include <zorro/stb_sprintf.h>
#include <cmath>

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

    _tiles1 = engine.loadBitmap("tiles1.bmp");

    _pos.x = (320 - (_tiles1->width())) / 2.0f;
    _pos.y = (240 - (_tiles1->height())) / 2.0f;
    return true;
}

bool Game::onUpdate(zorro::IEngine& engine, double dT)
{
    if (engine.getKeyState(zorro::KeyID::Escape).down)
    {
        engine.quit();
        return true;
    }

    if (engine.getKeyState(zorro::KeyID::Left).down)
    {
        _pos.x -= 50*dT;
    }

    if (engine.getKeyState(zorro::KeyID::Right).down)
    {
        _pos.x += 50*dT;
    }

    if (engine.getKeyState(zorro::KeyID::Up).down)
    {
        _pos.y -= 50*dT;
    }

    if (engine.getKeyState(zorro::KeyID::Down).down)
    {
        _pos.y += 50*dT;
    }

    char buffer[64];
    stbsp_snprintf(buffer, sizeof(buffer), "x=%0.2f y=%0.2f", _pos.x, _pos.y);
    engine.setDebugText(buffer);
    return true;
}

bool Game::onRender(zorro::IEngine& engine, double lag)
{
    engine.clearScreen(128);
    _tiles1->blt(std::round(_pos.x), std::round(_pos.y), 0, 0, _tiles1->width(), _tiles1->height(), 195);
    return true;
}

void Game::onCleanup(zorro::IEngine& engine)
{
}

zorro::IGame* zorro::makeGame()
{
    return new Game;
}

