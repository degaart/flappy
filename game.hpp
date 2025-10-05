#pragma once

#include <zorro/IEngine.hpp>
#include <zorro/IGame.hpp>
#include <zorro/rng.hpp>
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
    SpriteSheet _tiles1;
    zorro::IBitmap* _background;
    float _backgroundOffset;
    zorro::IBitmap* _ground;
    float _groundOffset;
    SpriteSheet _tiles2;
    float _accel;
    float _vel;
    zorro::Point<float> _pos;
    zorro::ISfx* _wingSfx;
    zorro::Rng _rng;
    float _pipeTimer;
    std::list<Pipe> _pipes;
    float _minGap;

    static constexpr auto SCREEN_WIDTH = 320.0f;
    static constexpr auto SCREEN_HEIGHT = 240.0f;
    static constexpr auto BACKGROUND_SPEED = 12.5f;
    static constexpr auto GROUND_SPEED = 50.0f;
    static constexpr auto PIPE_SPEED = 50.0f;
    static constexpr auto PIPE_RATE_MIN = 2.0f;
    static constexpr auto PIPE_RATE = 5.0f;

    float rand(float min, float max);
};

