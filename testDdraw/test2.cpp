#include "util.hpp"

#include <algorithm>
#include <assert.h>
#include <ddraw.h>
#include <math.h>
#include <stdint.h>

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

    std::optional<LRESULT> onEvent(HWND, UINT, WPARAM, LPARAM) override;
    void onPaint(WPARAM, LPARAM);
    void update(double dT);
    void render();
    LPDIRECTDRAWSURFACE4 loadBitmap(const char* name);
};

void __blit(const char* file, int line, LPDIRECTDRAWSURFACE4 dstSurf, LPRECT dstRect, LPDIRECTDRAWSURFACE4 srcSurf, LPRECT srcRect, DWORD flags, LPDDBLTFX fx);
#define blit(dstSurf, dstRect, srcSurf, srcRect, flags, fx) __blit(__FILE__, __LINE__, dstSurf, dstRect, srcSurf, srcRect, flags, fx)

App::App(HINSTANCE hInstance)
    : IApp(hInstance), _running(false), _b(0.0f)
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
    RECT rcWindow;
    rcWindow.left = 0;
    rcWindow.top = 0;
    rcWindow.right = 640;
    rcWindow.bottom = 480;
    AdjustWindowRect(&rcWindow, windowStyle, FALSE);

    HWND hwnd = CreateWindowEx(0, 
                               "MainWin", 
                               "Zinzolu", 
                               windowStyle, 
                               CW_USEDEFAULT, 0, 
                               rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, 
                               nullptr,
                               nullptr, 
                               _hinstance, 
                               this);
    if (!hwnd)
    {
        return false;
    }

    LPDIRECTDRAW ddraw;
    CHECK(DirectDrawCreate(nullptr, &ddraw, nullptr));
    CHECK(ddraw->QueryInterface(IID_IDirectDraw4, (void**)&_ddraw));
    CHECK(ddraw->SetCooperativeLevel(hwnd, DDSCL_NORMAL));

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    CHECK(_ddraw->CreateSurface(&ddsd, &_primarySurf, nullptr));

    RECT cltRect;
    GetClientRect(hwnd, &cltRect);

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = cltRect.right - cltRect.left;
    ddsd.dwHeight = cltRect.bottom - cltRect.top;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    CHECK(_ddraw->CreateSurface(&ddsd, &_backSurf, nullptr));

    CHECK(_ddraw->CreateClipper(0, &_clipper, nullptr));
    CHECK(_clipper->SetHWnd(0, hwnd));
    CHECK(_primarySurf->SetClipper(_clipper));

    _background = loadBitmap("doge.png");
    //auto spriteSheet = loadBitmap("tiles1.png");

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

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
                // trace("Updating, dT=%0.2f lag=%0.2f", dT, lag);
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
    // while (GetMessage(&msg, nullptr, 0, 0))
    //{
    //     TranslateMessage(&msg);
    //     DispatchMessage(&msg);
    // }
    return 0;
}

void App::cleanup()
{
    if (_backSurf)
    {
        _backSurf->Release();
        _backSurf = nullptr;
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
            SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        }
        else if (wparam == VK_F5)
        {
            render();
        }
        return 0;
        // case WM_PAINT:
        //     onPaint(wparam, lparam);
        //     return 0;
    }
    return std::nullopt;
}

void App::onPaint(WPARAM wparam, LPARAM lparam)
{
    // if (_running)
    //{
    //     assert(_primarySurf != nullptr);
    //     assert(_backSurf != nullptr);
    //     render();
    // }
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

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    CHECK(_backSurf->GetSurfaceDesc(&ddsd));

    auto backsurfWidth = ddsd.dwWidth;
    auto backsurfHeight = ddsd.dwHeight;
    if (backsurfWidth != rc.right - rc.left || backsurfHeight != rc.bottom - rc.top)
    {
        _backSurf->Release();

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

    /* Clear backbuffer */
    DDBLTFX fx;
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = 0xFFFFFFFF;
    CHECK(_backSurf->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fx));

    RECT dstRect;
    dstRect.left = 0;
    dstRect.top = 0;
    dstRect.right = 100;
    dstRect.bottom = 100;

    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = 100;
    srcRect.bottom = 100;
    assert(_background != nullptr);

    //CHECK(_backSurf->Blt(&dstRect, _background, nullptr, DDBLT_WAIT, nullptr));
    if (auto ret = _backSurf->Blt(&dstRect, _background, &srcRect, DDBLT_WAIT, nullptr); FAILED(ret))
    {
        trace("Blit failed: 0x%X %s. Rect: %ux%u %ux%u. Surface size: %ux%u",
              ret, hresult2str(ret),
              dstRect.left, dstRect.top,
              dstRect.right, dstRect.bottom,
              backsurfWidth, backsurfHeight);
    }

#if 0
    /* Render pattern */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    CHECK(_backSurf->Lock(nullptr, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, nullptr));

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = roundf(_b);
    auto ptr = reinterpret_cast<uint8_t*>(ddsd.lpSurface);

    for (int y = 0; y < ddsd.dwHeight; y++)
    {
        uint32_t* line = reinterpret_cast<uint32_t*>(ptr);
        for (int x = 0; x < ddsd.dwWidth; x++)
        {
            *line = b | (g << 8) | (r << 16) | (0xFF << 24);
            line++;
            r++;
        }
        g++;
        ptr = reinterpret_cast<uint8_t*>(ptr) + ddsd.lPitch;
    }
    CHECK(_backSurf->Unlock(nullptr));
#endif

    stbsp_snprintf(debugText, sizeof(debugText), "fps=%d w=%ld h=%ld", _fps, ddsd.dwWidth, ddsd.dwHeight);

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
    int width, height, bytesPerPixel;
    unsigned char* data = stbi_load(name, &width, &height, &bytesPerPixel, 0);
    assert(data != nullptr);
    assert(bytesPerPixel == 3);

    DDPIXELFORMAT ppf;
    ppf.dwSize = sizeof(ppf);
    CHECK(_primarySurf->GetPixelFormat(&ppf));
    trace("dwFlags: 0x%X", ppf.dwFlags);
    trace("dwRGBBitCount: %u", ppf.dwRGBBitCount);
    trace("dwRBitMask: 0x%X", ppf.dwRBitMask);
    trace("dwGBitMask: 0x%X", ppf.dwGBitMask);
    trace("dwBBitMask: 0x%X", ppf.dwBBitMask);
    trace("dwRGBAlphaBitMask: 0x%X", ppf.dwRGBAlphaBitMask);

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PIXELFORMAT;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    ddsd.ddpfPixelFormat.dwSize = sizeof(ddsd.ddpfPixelFormat);
    ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
    //ddsd.ddpfPixelFormat.dwRGBBitCount = 32;
    //ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
    //ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
    //ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
    //ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x0;
    ddsd.ddpfPixelFormat.dwRGBBitCount = 24;
    ddsd.ddpfPixelFormat.dwRBitMask = 0xFF0000;
    ddsd.ddpfPixelFormat.dwGBitMask = 0xFF00;
    ddsd.ddpfPixelFormat.dwBBitMask = 0xFF;
    ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0x0;

    LPDIRECTDRAWSURFACE4 surf;
    CHECK(_ddraw->CreateSurface(&ddsd, &surf, nullptr));

    DDPIXELFORMAT pf;
    pf.dwSize = sizeof(pf);
    surf->GetPixelFormat(&pf);
    trace("dwRGBBitCount: %u", pf.dwRGBBitCount);

    CHECK(surf->Lock(nullptr, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, nullptr));

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0xFF;

    // uint8_t* dstPtr = reinterpret_cast<uint8_t*>(ddsd.lpSurface);
    // for (int y = 0; y < height; y++)
    // {
    //     uint32_t* scanline = reinterpret_cast<uint32_t*>(dstPtr);
    //     for (int x = 0; x < width; x++)
    //     {
    //         *scanline = b | (g << 8) | (r << 16) | (0xFF << 24);
    //         scanline++;
    //     }
    //     dstPtr += ddsd.lPitch;
    // }

    uint8_t* dstPtr = reinterpret_cast<uint8_t*>(ddsd.lpSurface);
    for (int y = 0; y < height; y++)
    {
        uint8_t* scanline = reinterpret_cast<uint8_t*>(dstPtr);
        for (int x = 0; x < width; x++)
        {
            *scanline++ = b;
            *scanline++ = g;
            *scanline++ = r;
        }
        dstPtr += ddsd.lPitch;
    }

    //if (ddsd.lPitch == width * 3)
    //{
    //    memcpy(ddsd.lpSurface, data, width * height * 3);
    //}
    //else
    //{
    //    uint8_t* dstPtr = reinterpret_cast<uint8_t*>(ddsd.lpSurface);
    //    const uint8_t* srcPtr = reinterpret_cast<const uint8_t*>(data);
    //    for (int y = 0; y < height; y++)
    //    {
    //        memcpy(dstPtr, srcPtr, width * 3);
    //        dstPtr += ddsd.lPitch;
    //        srcPtr += width * 3;
    //    }
    //}

    CHECK(surf->Unlock(nullptr));
    stbi_image_free(data);

    return surf;
}

std::unique_ptr<IApp> makeApp(HINSTANCE hInstance)
{
    return std::make_unique<App>(hInstance);
}

