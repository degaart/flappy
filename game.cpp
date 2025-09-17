#include "game.hpp"
#include "engine.hpp"
#include <stdio.h>

const char* Game::getName()
{
    return "Flappy";
}

glm::ivec2 Game::getGameScreenSize()
{
    return {320, 240};
}

bool Game::onInit(Engine& engine)
{
    auto palette = engine.loadPalette("doge.pal");
    engine.setPalette(palette);

    _doge = engine.loadBitmap("doge.png");
    _tiles1 = engine.loadBitmap("tiles1.png");
    return true;
}

bool Game::onUpdate(Engine& engine, float dT)
{
    _offsetX = std::round(sinf(engine.getTime()) * 10.0f);
    _offsetY = std::round(cosf(engine.getTime()) * 10.0f);
    return true;
}

bool Game::onRender(Engine& engine, float lag)
{
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "offsetx=%d offsety=%d", _offsetX, _offsetY);
    engine.setDebugText(buffer);

    engine.blit(_doge, 0, 0, _doge.w, _doge.h, 0, 0);
    engine.blit(_tiles1, _offsetX, _offsetY, _tiles1.w, _tiles1.h, 0, 0, 195);
    return true;
}

