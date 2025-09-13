#pragma once

#include "engine.hpp"
#include <glm/glm.hpp>

class Game
{
public:
    static const char* getName();
    static glm::ivec2 getGameScreenSize();

    bool onInit(Engine& engine);
    bool onUpdate(Engine& engine, double dT);
    bool onRender(Engine& engine);
private:
    Bitmap _doge;
};

