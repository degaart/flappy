#include "game.hpp"
#include "engine.hpp"

const char* Game::getName()
{
    return "Flappy";
}

glm::ivec2 Game::getGameScreenSize()
{
    return {640, 480};
}

bool Game::onInit(Engine& engine)
{
    auto palette = engine.loadPalette("doge.pal");
    engine.setPalette(palette);

    _doge = engine.loadBitmap("doge.png", 640, 480);
    return true;
}

bool Game::onUpdate(Engine& engine, float dT)
{
    _offsetX = std::round(sinf(engine.getTime()) * 160.0f);
    _offsetY = std::round(cosf(engine.getTime()) * 120.0f);
    return true;
}

bool Game::onRender(Engine& engine, float lag)
{
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "offsetx=%d offsety=%d", _offsetX, _offsetY);
    engine.setDebugText(buffer);

    engine.blit(_doge, _offsetX, _offsetY, _doge.w, _doge.h, 0, 0);
    return true;
}

