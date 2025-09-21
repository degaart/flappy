#include "util.hpp"

#include <algorithm>
#include <assert.h>
#include <cstring>
#include <ddraw.h>
#include <math.h>
#include <memory>
#include <stdint.h>
#include <windows.h>
#include <winerror.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../stb_image.h"

class App : public IApp
{
public:
    explicit App(HINSTANCE hInstance);
    bool init() override;
    int run() override;
    void cleanup() override;

private:
    LPDIRECTDRAW4 _ddraw;
    LPDIRECTDRAWSURFACE4 _primarySurf;
    LPDIRECTDRAWSURFACE4 _backSurf;
    LPDIRECTDRAWCLIPPER _clipper;
    bool _running;
    CRITICAL_SECTION _criticalSection;
    float _b;
    int _fps;
    LPDIRECTDRAWSURFACE4 _background;
    LPDIRECTDRAWSURFACE4 _tiles1;
    LPDIRECTDRAWPALETTE _palette;
    std::vector<PALETTEENTRY> _paletteEntries;
    bool _fullscreen;

    std::optional<LRESULT> onEvent(HWND, UINT, WPARAM, LPARAM) override;
    void onPaint(WPARAM, LPARAM);
    void update(double dT);
    void render();
    void onZoom();
    LPDIRECTDRAWSURFACE4 loadBitmap(const char* name);
};

void __blit(const char* file, int line, LPDIRECTDRAWSURFACE4 dstSurf, LPRECT dstRect, LPDIRECTDRAWSURFACE4 srcSurf, LPRECT srcRect, DWORD flags, LPDDBLTFX fx);
#define blit(dstSurf, dstRect, srcSurf, srcRect, flags, fx) __blit(__FILE__, __LINE__, dstSurf, dstRect, srcSurf, srcRect, flags, fx)

App::App(HINSTANCE hInstance)
    : IApp(hInstance), _running(false), _b(0.0f), _palette(nullptr), _fullscreen(true)
{
    InitializeCriticalSection(&_criticalSection);
}

bool App::init()
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.hInstance = _hinstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = nullptr;
    wc.lpfnWndProc = IApp::windowProc;
    wc.lpszClassName = "MainWin";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    auto windowStyle = WS_OVERLAPPEDWINDOW;
    if (_fullscreen)
    {
        windowStyle = WS_POPUP;
    }
    RECT rcWindow;
    rcWindow.left = 0;
    rcWindow.top = 0;
    rcWindow.right = 640;
    rcWindow.bottom = 480;
    AdjustWindowRect(&rcWindow, windowStyle, FALSE);

    HWND hwnd = CreateWindowEx(0, "MainWin", "Flappy", windowStyle, 0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, nullptr,
                               nullptr, _hinstance, this);
    if (!hwnd)
    {
        return false;
    }

    trace("Loading palette from doge.pal...");
    _paletteEntries = loadPalette("doge.pal");
    assert(_paletteEntries.size() == 256);

    /* Adjust palette so the first and last 10 use windows' system palette colors */
    for (int i = 0; i < 10; i++)
    {
        _paletteEntries[i].peFlags = PC_EXPLICIT;
        _paletteEntries[i].peRed = i;
        _paletteEntries[i].peGreen = _paletteEntries[i].peBlue = 0;

        _paletteEntries[i+246].peFlags = PC_EXPLICIT;
        _paletteEntries[i+246].peRed = i+246;
        _paletteEntries[i+246].peGreen = _paletteEntries[i+246].peBlue = 0;
    }

    LPDIRECTDRAW ddraw;
    CHECK(DirectDrawCreate(nullptr, &ddraw, nullptr));
    CHECK(ddraw->QueryInterface(IID_IDirectDraw4, (void**)&_ddraw));

    if (_fullscreen)
    {
        CHECK(ddraw->SetCooperativeLevel(hwnd, DDSCL_FULLSCREEN|DDSCL_EXCLUSIVE|DDSCL_ALLOWREBOOT));
        CHECK(ddraw->SetDisplayMode(640, 480, 8));
    }
    else
    {
        CHECK(ddraw->SetCooperativeLevel(hwnd, DDSCL_NORMAL));
    }

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    CHECK(_ddraw->CreateSurface(&ddsd, &_primarySurf, nullptr));

    DDPIXELFORMAT pf;
    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    CHECK(_primarySurf->GetPixelFormat(&pf));
    if (pf.dwFlags & DDPF_PALETTEINDEXED8)
    {
        trace("Creating palette");
        CHECK(_ddraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE, _paletteEntries.data(), &_palette, nullptr));

        trace("Setting palette for primary surface");
        CHECK(_primarySurf->SetPalette(_palette));
    }
    else if ((pf.dwFlags & DDPF_RGB) == 0)
    {
        panic("Unsupported pixel format");
    }

    RECT cltRect;
    GetClientRect(hwnd, &cltRect);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = cltRect.right - cltRect.left;
    ddsd.dwHeight = cltRect.bottom - cltRect.top;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    CHECK(_ddraw->CreateSurface(&ddsd, &_backSurf, nullptr));
    if (_palette)
    {
        CHECK(_backSurf->SetPalette(_palette));
    }

    CHECK(_ddraw->CreateClipper(0, &_clipper, nullptr));
    CHECK(_clipper->SetHWnd(0, hwnd));
    CHECK(_primarySurf->SetClipper(_clipper));

    trace("Loading swatch.dat");
    _background = loadBitmap("swatch.dat");
    trace("Loading tiles1.dat");
    _tiles1 = loadBitmap("tiles1.dat");

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    trace("Initialization done");
    _running = true;
    return true;
}

int App::run()
{
    double prevTime = 0.0;
    double lag = 0;
    double frameTimer = 0.0f;
    int frames = 0;
    while (true)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                return msg.wParam;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        if (_running)
        {
            auto beginTime = getCurrentTime();
            auto elapsed = beginTime - prevTime;
            lag += elapsed;
            if (lag > 1.0)
            {
                lag = 1.0;
            }

            auto dT = 1.0 / 60.0;
            while (lag > dT)
            {
                update(dT);
                lag -= dT;
            }

            frames++;
            frameTimer += elapsed;
            if (frameTimer >= 1.0)
            {
                _fps = round(frames / frameTimer);
                frameTimer = 0;
                frames = 0;
            }

            render();
            prevTime = beginTime;
        }
    }
    return 0;
}

void App::cleanup()
{
    if (_tiles1)
    {
        _tiles1->Release();
        _tiles1 = nullptr;
    }

    if (_background)
    {
        _background->Release();
        _background = nullptr;
    }

    if (_backSurf)
    {
        _backSurf->Release();
        _backSurf = nullptr;
    }

    if (_palette)
    {
        _palette->Release();
        _palette = nullptr;
    }

    if (_ddraw)
    {
        _ddraw->Release();
        _ddraw = nullptr;
    }
    DeleteCriticalSection(&_criticalSection);
}

std::optional<LRESULT> App::onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        _running = false;
        DestroyWindow(hwnd);
        return 0;
    case WM_ACTIVATE:
        _running = (LOWORD(wparam) != 0);
        return 0;
    case WM_KEYUP:
        if (wparam == VK_ESCAPE)
        {
            SendMessage(hwnd, WM_CLOSE, 0, 0);
        }
        else if (wparam == VK_F5)
        {
            render();
        }
        else if (wparam == VK_F3)
        {
            onZoom();
        }
        return 0;
    }
    return std::nullopt;
}

void App::onPaint(WPARAM wparam, LPARAM lparam)
{
}

void App::onZoom()
{
    RECT rc;
    GetClientRect(_hwnd, &rc);

    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width > 640 || height > 480)
    {
        width = 640;
        height = 480;
    }
    else if (width > 320 || height > 240)
    {
        width = 320;
        height = 240;
    }
    else
    {
        width = 640;
        height = 480;
    }

    rc.left = 0;
    rc.right = width;
    rc.top = 0;
    rc.bottom = height;
    AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
    SetWindowPos(_hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE|SWP_NOZORDER);
}

void App::update(double dT)
{
    _b += (dT * 128.0f);
    while (_b > 255.0f)
    {
        _b -= 255.0f;
    }
}

void App::render()
{
    EnterCriticalSection(&_criticalSection);

    char debugText[255];
    *debugText = '\0';

    POINT origin;
    origin.x = origin.y = 0;
    ClientToScreen(_hwnd, &origin);

    RECT rc;
    GetClientRect(_hwnd, &rc);
    if (rc.right - rc.left <= 0 || rc.bottom - rc.top <= 0)
    {
        LeaveCriticalSection(&_criticalSection);
        return;
    }

    OffsetRect(&rc, origin.x, origin.y);
    if (rc.left < 0)
    {
        rc.left = 0;
    }
    if (rc.top < 0)
    {
        rc.top = 0;
    }

    auto backsurfSize = getSurfaceSize(_backSurf);
    if (backsurfSize.width != rc.right - rc.left || backsurfSize.height != rc.bottom - rc.top)
    {
        _backSurf->Release();

        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.dwWidth = rc.right - rc.left;
        ddsd.dwHeight = rc.bottom - rc.top;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        CHECK(_ddraw->CreateSurface(&ddsd, &_backSurf, nullptr));
    }

    if (_backSurf->IsLost())
    {
        CHECK(_backSurf->Restore());
    }

    backsurfSize = getSurfaceSize(_backSurf);

    /* Clear backbuffer */
    DDBLTFX fx;
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = 0xFFFFFFFF;
    CHECK(_backSurf->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fx));

    RECT dstRect;
    dstRect.left = 0;
    dstRect.top = 0;
    dstRect.right = std::min(640, backsurfSize.width);
    dstRect.bottom = std::min(480, backsurfSize.height);

    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = dstRect.right;
    srcRect.bottom = dstRect.bottom;
    assert(_background != nullptr);

    if (_background->IsLost())
    {
        CHECK(_background->Restore());
    }
    CHECK(_backSurf->BltFast(dstRect.left, dstRect.top, _background, &srcRect, DDBLTFAST_WAIT));

    stbsp_snprintf(debugText, sizeof(debugText), "fps=%d w=%d h=%d", _fps, backsurfSize.width, backsurfSize.height);

    HDC hdc;
    CHECK(_backSurf->GetDC(&hdc));
    SetTextColor(hdc, RGB(255, 0, 0));
    TextOut(hdc, 0, 0, debugText, lstrlen(debugText));
    _backSurf->ReleaseDC(hdc);

    /* Blit backbuffer to main surface */
    if (_primarySurf->IsLost())
    {
        CHECK(_primarySurf->Restore());
    }
    _primarySurf->Blt(&rc, _backSurf, nullptr, DDBLT_WAIT, nullptr);

    LeaveCriticalSection(&_criticalSection);
}

LPDIRECTDRAWSURFACE4 App::loadBitmap(const char* name)
{
    FILE* f = fopen(name, "rb");
    if (!f)
    {
        panic("Failed to open file %s", name);
    }

    char debugInfo[16];
    if (fread(debugInfo, sizeof(debugInfo), 1, f) != 1)
    {
        panic("Read error from %s", name);
    }
    trace("%s: %s", name, debugInfo);

    int width;
    if (fread(&width, sizeof(width), 1, f) != 1)
    {
        panic("Read error from %s", name);
    }

    int height;
    if (fread(&height, sizeof(height), 1, f) != 1)
    {
        panic("Read error from %s", name);
    }
    trace("%s: %dx%d", name, width, height);

    std::vector<uint8_t> data(width * height);
    if (fread(data.data(), 1, data.size(), f) != data.size())
    {
        panic("Read error from %s", name);
    }

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    LPDIRECTDRAWSURFACE4 surf;
    CHECK(_ddraw->CreateSurface(&ddsd, &surf, nullptr));
    if (_palette)
    {
        trace("Tralalero tralala");
        CHECK(surf->SetPalette(_palette));
    }

    CHECK(surf->Lock(nullptr, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, nullptr));
    if (ddsd.ddpfPixelFormat.dwRGBBitCount == 8)
    {
        uint8_t* dstPtr = reinterpret_cast<uint8_t*>(ddsd.lpSurface);
        const uint8_t* srcPtr = data.data();
        for (int y = 0; y < height; y++)
        {
            uint8_t* scanline = dstPtr;
            memcpy(scanline, srcPtr, width);
            dstPtr += ddsd.lPitch;
            srcPtr += width;
        }
    }
    else if (ddsd.ddpfPixelFormat.dwRGBBitCount == 16)
    {
        uint8_t* dstPtr = reinterpret_cast<uint8_t*>(ddsd.lpSurface);
        const uint8_t* srcPtr = data.data();
        for (int y = 0; y < height; y++)
        {
            uint16_t* scanline = reinterpret_cast<uint16_t*>(dstPtr);
            for (int x = 0; x < width; x++)
            {
                auto color = _paletteEntries[*srcPtr++];
                uint32_t r = color.peRed;
                uint32_t g = color.peGreen;
                uint32_t b = color.peBlue;
                *scanline = (b >> 3) | ((g >> 2) << 5) | ((r >> 3) << 11);
                scanline++;
            }
            dstPtr += ddsd.lPitch;
        }
    }
    else if (ddsd.ddpfPixelFormat.dwRGBBitCount == 24)
    {
        uint8_t* dstPtr = reinterpret_cast<uint8_t*>(ddsd.lpSurface);
        const uint8_t* srcPtr = data.data();
        for (int y = 0; y < height; y++)
        {
            uint8_t* scanline = reinterpret_cast<uint8_t*>(dstPtr);
            for (int x = 0; x < width; x++)
            {
                auto color = _paletteEntries[*srcPtr++];
                *scanline++ = color.peBlue;
                *scanline++ = color.peGreen;
                *scanline++ = color.peRed;
            }

            dstPtr += ddsd.lPitch;
        }
    }
    else if (ddsd.ddpfPixelFormat.dwRGBBitCount == 32)
    {
        uint8_t* dstPtr = reinterpret_cast<uint8_t*>(ddsd.lpSurface);
        const uint8_t* srcPtr = data.data();
        for (int y = 0; y < height; y++)
        {
            uint32_t* scanline = reinterpret_cast<uint32_t*>(dstPtr);
            for (int x = 0; x < width; x++)
            {
                auto color = _paletteEntries[*srcPtr++];
                *scanline++ = color.peBlue | (color.peGreen << 8) | (color.peRed << 16);
            }
            dstPtr += ddsd.lPitch;
        }
    }
    else
    {
        panic("Not implemented yet");
    }

    CHECK(surf->Unlock(nullptr));

    return surf;
}

std::unique_ptr<IApp> makeApp(HINSTANCE hInstance)
{
    return std::make_unique<App>(hInstance);
}

