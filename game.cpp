#include "game.hpp"

#include <assert.h>
#include <cmath>
#include <windows.h>
#include <zorro/stb_sprintf.h>
#include <zorro/util.hpp>

IdleState Game::_idleState;
RunningState Game::_runningState;
GameOverState Game::_gameOverState;

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

    _ground = engine.loadBitmap("ground.bmp");
    _groundY = SCREEN_HEIGHT - _ground->height();

    _tiles2._bitmap = engine.loadBitmap("pipes.bmp");
    _tiles2._colorKey = 195;
    _tiles2.addImage(0, 0, _tiles2._bitmap->width() / 2, _tiles2._bitmap->height());
    _tiles2.addImage(_tiles2._bitmap->width() / 2, 0, _tiles2._bitmap->width() / 2, _tiles2._bitmap->height());

    _gameOver = engine.loadBitmap("gameover.bmp");
    _gameOverVisible = false;
    _message = engine.loadBitmap("message.bmp");
    _messageVisible = false;

    _wingSfx = engine.loadSfx("wing.ogg");
    _dieSfx = engine.loadSfx("die.ogg");
    _pointSfx = engine.loadSfx("point.ogg");

    _state = nullptr;
    _nextState = nullptr;
    setState(engine, &_idleState);

    return true;
}

bool Game::onUpdate(zorro::IEngine& engine, double dT)
{
    if (engine.getKeyState(zorro::KeyID::Escape).down)
    {
        engine.quit();
        return true;
    }

    if (_state)
    {
        _state->onUpdate(engine, *this, dT);
    }

    char debugText[64];
    stbsp_snprintf(debugText, sizeof(debugText), "score=%d", _score);
    engine.setDebugText(debugText);

    if (_nextState)
    {
        if (_state)
        {
            _state->onExit(engine, *this);
        }

        _state = _nextState;
        _nextState = nullptr;

        _state->onEnter(engine, *this);
    }

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
    offset = std::round(_groundOffset);
    while (groundW < SCREEN_WIDTH)
    {
        _ground->blt(groundW, _groundY, offset, 0, _ground->width() - offset, _ground->height());
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

    if (_gameOverVisible)
    {
        int x = (SCREEN_WIDTH - _gameOver->width()) / 2;
        int y = (SCREEN_HEIGHT - _gameOver->height()) / 2;
        _gameOver->blt(x, y, 0, 0, _gameOver->width(), _gameOver->height(), 195);
    }

    if (_messageVisible)
    {
        int x = (SCREEN_WIDTH - _message->width()) / 2;
        int y = (SCREEN_HEIGHT - _message->height()) / 2;
        _message->blt(x, y, 0, 0, _message->width(), _message->height(), 195);
    }

    return true;
}

void Game::onCleanup(zorro::IEngine& engine)
{
}

bool Game::checkCollision(const zorro::Rect<float>& a, const zorro::Rect<float>& b)
{
    return !(a.x + a.w <= b.x || b.x + b.w <= a.x || a.y + a.h <= b.y || b.y + b.h <= a.y);
}

void Game::setState(zorro::IEngine& engine, IState* newState)
{
    _nextState = newState;
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


void IdleState::onEnter(zorro::IEngine& engine, class Game& game)
{
    game._messageVisible = true;
    game._backgroundOffset = 0.0f;
    game._groundOffset = 0.0f;
    game._accel = 100.0f;
    game._vel = 0.0f;
    game._pos.x = 10.0f;
    game._pos.y = (240 - (game._tiles1._images[0].h)) / 2.0f;
    game._pipeTimer = Game::PIPE_RATE_MIN + (Game::PIPE_RATE * game._rng.fnext());
    game._minGap = game._tiles1._images[0].h * 4.0f;
    game._pipes.clear();
    game._score = 0;
}

void IdleState::onUpdate(zorro::IEngine& engine, class Game& game, double dT)
{
    if (auto state = engine.getKeyState(zorro::KeyID::Space); state.down && !state.repeat)
    {
        game._messageVisible = false;
        game.setState(engine, &Game::_runningState);
    }
}

void RunningState::onEnter(zorro::IEngine& engine, class Game& game)
{
}

void RunningState::onUpdate(zorro::IEngine& engine, class Game& game, double dT)
{
    if (engine.getKeyState(zorro::KeyID::Space).down)
    {
        if (game._vel > 10.0f)
        {
            game._vel = -110.0f;

            float modulation = game._rng.fnext() - 0.5f; /* 0.0f to 1.0f */
            int freq = 22050 + round(11025 * modulation);
            game._wingSfx->setFreq(freq);
            game._wingSfx->play();
        }
    }

    game._vel = game._vel + (game._accel * dT);
    game._pos.y = game._pos.y + (game._vel * dT);

    game._backgroundOffset += Game::BACKGROUND_SPEED * dT;
    while (game._backgroundOffset > game._background->width())
    {
        game._backgroundOffset -= game._background->width();
    }

    game._groundOffset += Game::GROUND_SPEED * dT;
    while (game._groundOffset > game._ground->width())
    {
        game._groundOffset -= game._ground->width();
    }

    // Generate bird hitbox
    zorro::Rect<float> birdHitbox;
    birdHitbox.x = game._pos.x + 1;
    birdHitbox.y = game._pos.y + 1;
    birdHitbox.w = game._tiles1._images[0].w - 2;
    birdHitbox.h = game._tiles1._images[0].h - 2;

    for (auto it = game._pipes.begin(), next_it = game._pipes.begin(); it != game._pipes.end(); it = next_it)
    {
        next_it = it;
        ++next_it;

        it->x -= Game::PIPE_SPEED * dT;
        if (it->x < -game._tiles2._images[0].w)
        {
            game._pipes.erase(it);
        }
        else if (!it->counted && it->x + game._tiles2._images[0].w < game._pos.x)
        {
            it->counted = true;
            game._score++;

            float modulation = game._rng.fnext() - 0.5f; /* 0.0f to 1.0f */
            int freq = 22050 + round(11025 * modulation);
            game._pointSfx->setFreq(freq);
            game._pointSfx->play();
        }
        else
        {
            zorro::Rect<float> pipeRect;
            pipeRect.x = it->x;
            pipeRect.y = 0.0f;
            pipeRect.w = game._tiles2._images[0].w;
            pipeRect.h = it->upperGap;
            if (Game::checkCollision(birdHitbox, pipeRect))
            {
                game.setState(engine, &Game::_gameOverState);
                break;
            }

            pipeRect.x = it->x;
            pipeRect.y = it->lowerGap;
            pipeRect.w = game._tiles2._images[0].w;
            pipeRect.h = Game::SCREEN_HEIGHT - it->lowerGap;
            if (Game::checkCollision(birdHitbox, pipeRect))
            {
                game.setState(engine, &Game::_gameOverState);
                break;
            }
        }
    }

    if (game._minGap > game._tiles1._images[0].h * 1.5f)
    {
        game._minGap -= dT / 5.0f;
    }

    if (game._pipeTimer > dT)
    {
        game._pipeTimer -= dT;
    }
    else
    {
        Pipe newPipe;
        newPipe.x = Game::SCREEN_WIDTH;
        newPipe.upperGap = std::round(game.rand(30.0f, (Game::SCREEN_HEIGHT - game._minGap) / 2));
        newPipe.lowerGap = std::round(game.rand(newPipe.upperGap + game._minGap, Game::SCREEN_HEIGHT - 30));
        newPipe.counted = false;
        game._pipes.push_back(newPipe);
        game._pipeTimer = Game::PIPE_RATE_MIN + (Game::PIPE_RATE * game._rng.fnext());
    }

    // Check ground collision
    zorro::Rect<float> groundHitbox;
    groundHitbox.x = 0;
    groundHitbox.y = game._groundY;
    groundHitbox.w = Game::SCREEN_WIDTH;
    groundHitbox.h = Game::SCREEN_HEIGHT - game._groundY;
    if (Game::checkCollision(birdHitbox, groundHitbox))
    {
        game.setState(engine, &Game::_gameOverState);
    }
}

void GameOverState::onEnter(zorro::IEngine& engine, class Game& game)
{
    _timer = 3.0f;
    game._dieSfx->play();
}

void GameOverState::onUpdate(zorro::IEngine& engine, class Game& game, double dT)
{
    _timer -= dT;
    if (_timer < 2.0f)
    {
        game._gameOverVisible = true;
    }
    if (_timer < 0.0f)
    {
        game._gameOverVisible = false;
        game.setState(engine, &Game::_idleState);
    }
}

