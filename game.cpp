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

bool Game::onUpdate(Engine& engine, double dT)
{
    return true;
}

bool Game::onRender(Engine& engine)
{
    engine.blit(_doge, 0, 0, _doge.w, _doge.h, 0, 0);
    return true;
}

