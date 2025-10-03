#pragma once

#include <zorro/IEngine.hpp>
#include <zorro/IGame.hpp>
#include <zorro/rng.hpp>

struct SpriteSheet
{
    zorro::IBitmap* _bitmap;
    int _width;
    int _height;
    int _imageCount;
    uint8_t _colorKey;

    void blt(int dstX, int dstY, int imageIndex);
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
    float _accel;
    float _vel;
    zorro::Point<float> _pos;
    zorro::ISfx* _wingSfx;
    zorro::Rng _rng;

    static constexpr auto SCREEN_WIDTH = 320.0f;
    static constexpr auto SCREEN_HEIGHT = 240.0f;
    static constexpr auto BACKGROUND_SPEED = 50.0f;
};

