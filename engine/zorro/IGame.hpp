#pragma once

#include "IEngine.hpp"

namespace zorro
{
    struct GameParams
    {
        const char* name;
        Size<int> size;
    };

    struct IGame
    {
        virtual ~IGame() = default;
        virtual GameParams getParams() const = 0;
        virtual bool onInit(IEngine& engine) = 0;
        virtual bool onUpdate(IEngine& engine, double dT) = 0;
        virtual bool onRender(IEngine& engine, double lag) = 0;
        virtual void onCleanup(IEngine& engine) = 0;
    };

    IGame* makeGame();
}

