#include "game.hpp"

#include <assert.h>
#include <cmath>
#include <windows.h>
#include <zorro/stb_sprintf.h>
#include <zorro/util.hpp>

float Game::rand(float min, float max)
{
    return min + ((max - min) * _rng.fnext());
}

zorro::GameParams Game::getParams() const
{
    return zorro::GameParams {"Flappy", {320, 240}};
}

bool Game::onInit(zorro::IEngine& engine)
{
    double now = engine.getTime();
    _rng.seed(*reinterpret_cast<uint64_t*>(&now));

    auto palette = engine.loadPalette("doge.pal");
    engine.setPalette(palette);

    _tiles1._bitmap = engine.loadBitmap("tiles1.bmp");
    _tiles1.addImage(0, 0, 34, 24);
    _tiles1.addImage(34, 0, 34, 24);
    _tiles1.addImage(68, 0, 34, 24);
    _tiles1._colorKey = 195;

    _background = engine.loadBitmap("background.bmp");
    _backgroundOffset = 0.0f;

    _ground = engine.loadBitmap("ground.bmp");
    _groundOffset = 0.0f;

    _tiles2._bitmap = engine.loadBitmap("pipes.bmp");
    _tiles2._colorKey = 195;
    _tiles2.addImage(0, 0, _tiles2._bitmap->width()/2, _tiles2._bitmap->height());
    _tiles2.addImage(_tiles2._bitmap->width()/2, 0, _tiles2._bitmap->width()/2, _tiles2._bitmap->height());

    _accel = 100.0f;
    _vel = 0.0f;
    _pos.x = 10.0f;
    _pos.y = (240 - (_tiles1._images[0].h)) / 2.0f;

    _wingSfx = engine.loadSfx("wing.ogg");

    _pipeTimer = PIPE_RATE_MIN + (PIPE_RATE * _rng.fnext());
    _minGap = _tiles1._images[0].h * 4.0f;

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

            float modulation = _rng.fnext() - 0.5f; /* 0.0f to 1.0f */
            int freq = 22050 + round(11025 * modulation);
            _wingSfx->setFreq(freq);
            _wingSfx->play();
        }
    }

    _vel = _vel + (_accel * dT);
    _pos.y = _pos.y + (_vel * dT);

    _backgroundOffset += BACKGROUND_SPEED * dT;
    while (_backgroundOffset > _background->width())
    {
        _backgroundOffset -= _background->width();
    }

    _groundOffset += GROUND_SPEED * dT;
    while (_groundOffset > _ground->width())
    {
        _groundOffset -= _ground->width();
    }

    for (auto it = _pipes.begin(), next_it = _pipes.begin(); it != _pipes.end(); it = next_it)
    {
        next_it = it;
        ++next_it;

        it->x -= PIPE_SPEED * dT;
        if (it->x < -_tiles2._images[0].w)
        {
            _pipes.erase(it);
        }
    }

    if (_minGap > _tiles1._images[0].h * 1.5f)
    {
        _minGap -= dT / 5.0f;
    }

    if (_pipeTimer > dT)
    {
        _pipeTimer -= dT;
    }
    else
    {
        Pipe newPipe;
        newPipe.x = SCREEN_WIDTH;
        newPipe.upperGap = std::round(rand(30.0f, (SCREEN_HEIGHT - _minGap) / 2));
        newPipe.lowerGap = std::round(rand(newPipe.upperGap + _minGap, SCREEN_HEIGHT - 30));
        _pipes.push_back(newPipe);
        _pipeTimer = PIPE_RATE_MIN + (PIPE_RATE * _rng.fnext());
    }

    char debugText[64];
    stbsp_snprintf(debugText, sizeof(debugText), "minGap=%0.2f", _minGap);
    engine.setDebugText(debugText);
    return true;
}

bool Game::onRender(zorro::IEngine& engine, double lag)
{
    int backgroundW = 0;
    int offset = std::round(_backgroundOffset);
    while (backgroundW < SCREEN_WIDTH)
    {
        _background->blt(backgroundW, 0, offset, 0, _background->width() - offset, _background->height());
        backgroundW += _background->width() - offset;
        offset = 0;
    }

    int groundW = 0;
    int groundY = SCREEN_HEIGHT - _ground->height();
    offset = std::round(_groundOffset);
    while (groundW < SCREEN_WIDTH)
    {
        _ground->blt(groundW, groundY, offset, 0, _ground->width() - offset, _ground->height());
        groundW += _ground->width() - offset;
        offset = 0;
    }

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

    static bool first = true;
    for (const auto& pipe : _pipes)
    {
        _tiles2.blt(std::round(pipe.x), pipe.upperGap - _tiles2._images[1].h - 1, 1);
        _tiles2.blt(std::round(pipe.x), pipe.lowerGap, 0);
    }

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
    auto rect = _images[imageIndex];
    _bitmap->blt(dstX, dstY, rect.x, rect.y, rect.w, rect.h, _colorKey);
}

void SpriteSheet::addImage(int x, int y, int w, int h)
{
    zorro::Rect<int> rect;
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;
    _images.push_back(rect);
}

