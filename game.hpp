#pragma once

#include <zorro/IEngine.hpp>
#include <zorro/IGame.hpp>

class Game : public zorro::IGame
{
public:
    zorro::GameParams getParams() const override;
    bool onInit(zorro::IEngine& engine) override;
    bool onUpdate(zorro::IEngine& engine, double dT) override;
    bool onRender(zorro::IEngine& engine, double lag) override;
    void onCleanup(zorro::IEngine& engine) override;
private:
    zorro::IBitmap* _tiles1;
    zorro::Point<float> _pos;
};

