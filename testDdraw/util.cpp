
INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT)
{
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq))
    {
        panic("QueryPerformanceFrequency failed");
    }
    hrTimerFrequency = freq.QuadPart;

    CoInitialize(nullptr);

    auto app = makeApp(hInstance);
    if (!app->init())
    {
        panic("App::init() failed");
    }

    auto ret = app->run();
    app->cleanup();
    CoUninitialize();
    return ret;
}

