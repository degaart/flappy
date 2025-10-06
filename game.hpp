#pragma once

#include <zorro/IEngine.hpp>
#include <zorro/IGame.hpp>
#include <zorro/rng.hpp>
#include <zorro/BufferView.hpp>
#include <vector>
#include <list>

struct SpriteSheet
{
    zorro::IBitmap* _bitmap;
    uint8_t _colorKey;
    std::vector<zorro::Rect<int>> _images;

    void addImage(int x, int y, int w, int h);
    void blt(int dstX, int dstY, int imageIndex);
};

struct Pipe
{
    float x;
    int upperGap;
    int lowerGap;
    bool counted;
};

class IState
{
public:
    virtual ~IState() = default;
    virtual void onEnter(zorro::IEngine& engine, class Game& game) {};
    virtual void onExit(zorro::IEngine& engine, class Game& game) {};
    virtual void onUpdate(zorro::IEngine& engine, class Game& game, double dT) {};
};

class IdleState : public IState
{
public:
    void onEnter(zorro::IEngine& engine, class Game& game) override;
    void onUpdate(zorro::IEngine& engine, class Game& game, double dT) override;
};

class RunningState : public IState
{
public:
    void onEnter(zorro::IEngine& engine, class Game& game) override;
    void onUpdate(zorro::IEngine& engine, class Game& game, double dT) override;
};

class GameOverState : public IState
{
public:
    void onEnter(zorro::IEngine& engine, class Game& game) override;
    void onUpdate(zorro::IEngine& engine, class Game& game, double dT) override;
private:
    float _timer;
};

class Game : public zorro::IGame
{
public:
    zorro::GameParams getParams() const override;
    bool onInit(zorro::IEngine& engine) override;
    bool onUpdate(zorro::IEngine& engine, double dT) override;
    bool onRender(zorro::IEngine& engine, double lag) override;
    void onCleanup(zorro::IEngine& engine) override;
private:
    friend class IdleState;
    friend class RunningState;
    friend class GameOverState;

    IState* _state;
    IState* _nextState;

    SpriteSheet _tiles1;
    zorro::IBitmap* _background;
    float _backgroundOffset;
    zorro::IBitmap* _ground;
    float _groundOffset;
    SpriteSheet _tiles2;
    zorro::IBitmap* _gameOver;
    bool _gameOverVisible;
    zorro::IBitmap* _message;
    bool _messageVisible;
    SpriteSheet _numbers;
    float _accel;
    float _vel;
    zorro::Point<float> _pos;
    zorro::ISfx* _wingSfx;
    zorro::ISfx* _dieSfx;
    zorro::ISfx* _pointSfx;
    zorro::Rng _rng;
    float _pipeTimer;
    std::list<Pipe> _pipes;
    float _minGap;
    float _groundY;
    int _score;

    static constexpr auto SCREEN_WIDTH = 320.0f;
    static constexpr auto SCREEN_HEIGHT = 240.0f;
    static constexpr auto BACKGROUND_SPEED = 12.5f;
    static constexpr auto GROUND_SPEED = 50.0f;
    static constexpr auto PIPE_SPEED = 50.0f;
    static constexpr auto PIPE_RATE_MIN = 2.0f;
    static constexpr auto PIPE_RATE = 5.0f;
    
    static IdleState _idleState;
    static RunningState _runningState;
    static GameOverState _gameOverState;

    float rand(float min, float max);
    static bool checkCollision(const zorro::Rect<float>& a, const zorro::Rect<float>& b);
    void setState(zorro::IEngine& engine, IState* newState);
    static zorro::BufferView loadAsset(const char* name);
};

