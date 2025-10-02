#include "engine.hpp"

INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nCmdShow)
{
    zorro::Engine* engine = new zorro::Engine(hInstance);
    return engine->run();
    return 0;
}

