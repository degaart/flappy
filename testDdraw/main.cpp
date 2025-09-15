#include <assert.h>
#include <ddraw.h>
#include <shlwapi.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

[[noreturn]] void __panic(const char* file, int line, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char baseFile[MAX_PATH];
    lstrcpyn(baseFile, file, sizeof(baseFile));
    PathStripPathA(baseFile);

    char buffer[512];
    snprintf(buffer, sizeof(buffer), "Fatal error at %s:%d\n\n", baseFile, line);
    vsnprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), format, args);
    va_end(args);

    MessageBoxA(nullptr, buffer, "Panic", MB_OK | MB_ICONERROR);
    ExitProcess(1);
}

#define panic(...) __panic(__FILE__, __LINE__, __VA_ARGS__)

class App
{
public:
    bool init(HINSTANCE hInstance);
    bool run();

private:
    HWND _hwnd;
    LPDIRECTDRAW4 _ddraw;
    LPDIRECTDRAWSURFACE4 _primarySurface;
    LPDIRECTDRAWSURFACE4 _backbuffer;
    LPDIRECTDRAWCLIPPER _clipper;

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    LRESULT onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    void onKeyUp(int keyCode, int scancode);
    void onDestroy();
    void render();
};

bool App::init(HINSTANCE hInstance)
{
    if (FAILED(CoInitialize(nullptr)))
    {
        panic("CoInitialize() failed");
    }

    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpfnWndProc = App::windowProc;
    wc.lpszClassName = "MainWin";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    HWND hwnd = CreateWindowEx(0, "MainWin", "Zinzolu", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, this);
    if (!hwnd)
    {
        return false;
    }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    LPDIRECTDRAW ddraw;
    if (FAILED(DirectDrawCreate(nullptr, &ddraw, nullptr)))
    {
        panic("DirectDrawCreate() failed");
    }
    else if (FAILED(ddraw->QueryInterface(IID_IDirectDraw4, (void**)&_ddraw)))
    {
        panic("QueryInterface failed");
    }
    ddraw->Release();

    if (FAILED(_ddraw->SetCooperativeLevel(hwnd, DDSCL_NORMAL)))
    {
        panic("SetCooperativeLevel() failed");
    }

    DDSURFACEDESC2 ddsd2;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    if (FAILED(_ddraw->GetDisplayMode(&ddsd2)))
    {
        panic("GetDisplayMode() failed");
    }

    int width, height, pitch;
    int bpp;
    char msg[512];
    snprintf(msg, sizeof(msg),
             "Dimensions: %lux%lu\n"
             "Pitch: %lu\n"
             "Bpp: %lu\n"
             "RMask: 0x%08lX\n"
             "GMask: 0x%08lX\n"
             "BMask: 0x%08lX\n"
             "Alpha: %s\n"
             "AlphaMask: 0x%08lX",
             ddsd2.dwWidth, ddsd2.dwHeight, ddsd2.lPitch, ddsd2.ddpfPixelFormat.dwRGBBitCount, ddsd2.ddpfPixelFormat.dwRBitMask,
             ddsd2.ddpfPixelFormat.dwGBitMask, ddsd2.ddpfPixelFormat.dwBBitMask, (ddsd2.ddpfPixelFormat.dwFlags & DDPF_ALPHAPIXELS) ? "true" : "false",
             ddsd2.ddpfPixelFormat.dwRGBAlphaBitMask);
    assert(ddsd2.ddpfPixelFormat.dwFlags & DDPF_RGB);
    printf("*** Surface info ***\n%s", msg);

    DDSURFACEDESC2 primarySD;
    memset(&primarySD, 0, sizeof(primarySD));
    primarySD.dwSize = sizeof(primarySD);
    primarySD.dwFlags = DDSD_CAPS;
    primarySD.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    if (FAILED(_ddraw->CreateSurface(&primarySD, &_primarySurface, nullptr)))
    {
        panic("CreateSurface failed");
    }

    if (FAILED(_ddraw->CreateClipper(0, &_clipper, nullptr)))
    {
        panic("CreateClipper() failed");
    }

    assert(hwnd != nullptr);
    if (FAILED(_clipper->SetHWnd(0, hwnd)))
    {
        panic("IDirectDrawClipper::SetHWnd() failed");
    }

    //if (FAILED(_primarySurface->SetClipper(_clipper)))
    //{
    //    panic("SetClipper() failed");
    //}
    //_clipper->Release();
    //_clipper = nullptr;

    DDSURFACEDESC2 bbsd;
    memset(&bbsd, 0, sizeof(bbsd));
    bbsd.dwSize = sizeof(bbsd);
    bbsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    bbsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    bbsd.dwWidth = 320;
    bbsd.dwHeight = 240;
    bbsd.ddpfPixelFormat.dwSize = sizeof(bbsd.ddpfPixelFormat);
    bbsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
    bbsd.ddpfPixelFormat.dwRGBBitCount = 8;
    if (auto ret = _ddraw->CreateSurface(&bbsd, &_backbuffer, nullptr); FAILED(ret))
    {
        panic("CreateSurface failed: 0x%X", ret);
    }

    return true;
}

bool App::run()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        render();
    }
    return msg.wParam == 0;
}

void App::render()
{
    if (_backbuffer == nullptr)
    {
        return;
    }

    DDSURFACEDESC2 ddsd2;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    if (FAILED(_backbuffer->Lock(nullptr, &ddsd2, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, nullptr)))
    {
        panic("IDirectDrawSurface4::Lock() failed");
    }

    uint8_t c = 0;
    for (int y = 0; y < ddsd2.dwHeight; y++)
    {
        uint8_t* ptr = (uint8_t*)ddsd2.lpSurface + (y * ddsd2.lPitch);
        for (int x = 0; x < ddsd2.dwWidth; x++)
        {
            *ptr = c;
            ptr++;
        }
        c++;
    }

    if (FAILED(_backbuffer->Unlock(nullptr)))
    {
        panic("IDirectDrawSurface4::Unlock() failed");
    }

    POINT origin;
    origin.x = origin.y = 0;
    ClientToScreen(_hwnd, &origin);

    RECT rc;
    GetClientRect(_hwnd, &rc);
    OffsetRect(&rc, origin.x, origin.y);

    HRESULT hResult;
    while (true)
    {
        hResult = _primarySurface->Blt(&rc, _backbuffer, nullptr, DDBLT_WAIT, nullptr);
        if (hResult == DDERR_SURFACELOST)
        {
            _primarySurface->Restore();
            _backbuffer->Restore();
        }
        else if (FAILED(hResult))
        {
            panic("Blt() failed");
        }
        else
        {
            break;
        }
    }
}

LRESULT CALLBACK App::windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    if (msg == WM_NCCREATE)
    {
        auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lparam);
#ifdef _WIN64
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)createStruct->lpCreateParams);
#else
        SetWindowLong(hwnd, GWL_USERDATA, (LONG)createStruct->lpCreateParams);
#endif

        App* app = reinterpret_cast<App*>(createStruct->lpCreateParams);
        app->_hwnd = hwnd;
    }
    else
    {
#ifdef _WIN64
        App* app = reinterpret_cast<App*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
#else
        App* app = reinterpret_cast<App*>(GetWindowLong(hwnd, GWL_USERDATA));
#endif
        if (app)
        {
            return app->onEvent(hwnd, msg, wparam, lparam);
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

LRESULT App::onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        onDestroy();
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    case WM_KEYUP:
        onKeyUp(static_cast<int>(wparam), (lparam >> 16) & 0xFF);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

void App::onKeyUp(int keyCode, int scancode)
{
    if (keyCode == VK_ESCAPE)
    {
        SendMessage(_hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
    }
    else if (keyCode == VK_F5)
    {
        render();
    }
}

void App::onDestroy()
{
    _clipper->Release();
    _clipper = nullptr;

    _backbuffer->Release();
    _backbuffer = nullptr;

    _primarySurface->Release();
    _primarySurface = nullptr;

    _ddraw->Release();
    _ddraw = nullptr;

    PostQuitMessage(0);
}

INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT)
{
    App app;
    if (!app.init(hInstance))
    {
        panic("App::init() failed");
    }

    if (!app.run())
    {
        CoUninitialize();
        return 1;
    }

    CoUninitialize();
    return 0;
}

