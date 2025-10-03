#pragma once

#include <zorro/IEngine.hpp>
#include <zorro/IGame.hpp>

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
    float _accel;
    float _vel;
    zorro::Point<float> _pos;
};

