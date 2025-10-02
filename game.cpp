#include "game.hpp"

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
    return true;
}

bool Game::onUpdate(zorro::IEngine& engine, double dT)
{
    if (engine.getKeyState(zorro::KeyID::Escape).down)
    {
        engine.quit();
        return true;
    }
    return true;
}

bool Game::onRender(zorro::IEngine& engine, double lag)
{
    engine.clearScreen(128);
    _tiles1->blt(0, 0, 0, 0, _tiles1->width(), _tiles1->height());
    return true;
}

void Game::onCleanup(zorro::IEngine& engine)
{
}

zorro::IGame* zorro::makeGame()
{
    return new Game;
}

