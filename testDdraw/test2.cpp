#include "fixed16.hpp"
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

/*
 * We have 4 categories of surfaces:
 *  - primarySurface: surface which is displayed on screen
 *  - backbuffer: surface with the same geometry as primary surface, where the worksurface will be rendered
 *  - worksurface: surface where we render. Fixed geometry of 320x240x8
 *  - offscreen surface: surface to be blitted to worksurface
 *
 *  As a special case for optimization: if backbuffer has geometry 320x240x8, we skip worksurface and render
 *  directly into it
 */

struct Bitmap
{
    int w, h;
    std::vector<uint8_t> ptr;
};

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
    Bitmap _background;
    Bitmap _tiles1;
    LPDIRECTDRAWPALETTE _palette;
    Bitmap _framebuffer;
    bool _running;
    CRITICAL_SECTION _criticalSection;
    float _b;
    int _fps;
    std::vector<PALETTEENTRY> _paletteEntries;
    bool _fullscreen;
    int _zoom;

    static constexpr auto BASE_WIDTH = 320;
    static constexpr auto BASE_HEIGHT = 240;

    static constexpr auto FULLSCREEN_STYLE = WS_POPUP;
    static constexpr auto WINDOWED_STYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    std::optional<LRESULT> onEvent(HWND, UINT, WPARAM, LPARAM) override;
    void onPaint(WPARAM, LPARAM);
    void update(double dT);
    void render();
    void onZoom(bool zoomIn);
    Bitmap loadBitmap(const char* name);
    void freeSurfaces();
    void createSurfaces();
};

void __blit(const char* file, int line, LPDIRECTDRAWSURFACE4 dstSurf, LPRECT dstRect, LPDIRECTDRAWSURFACE4 srcSurf, LPRECT srcRect, DWORD flags, LPDDBLTFX fx);
#define blit(dstSurf, dstRect, srcSurf, srcRect, flags, fx) __blit(__FILE__, __LINE__, dstSurf, dstRect, srcSurf, srcRect, flags, fx)

App::App(HINSTANCE hInstance)
    : IApp(hInstance), _ddraw(nullptr), _primarySurf(nullptr), _backSurf(nullptr), _palette(nullptr), _running(false), _b(0.0f), _fullscreen(false), _zoom(1)
{
    InitializeCriticalSection(&_criticalSection);
    _framebuffer.w = BASE_WIDTH;
    _framebuffer.h = BASE_HEIGHT;
    _framebuffer.ptr.insert(_framebuffer.ptr.end(), _framebuffer.w * _framebuffer.h, '\0');
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

    auto windowStyle = _fullscreen ? FULLSCREEN_STYLE : WINDOWED_STYLE;

    RECT rcWindow;
    rcWindow.left = 0;
    rcWindow.top = 0;
    rcWindow.right = BASE_WIDTH * _zoom;
    rcWindow.bottom = BASE_HEIGHT * _zoom;
    AdjustWindowRect(&rcWindow, windowStyle, FALSE);

    HWND hwnd = CreateWindowEx(0, "MainWin", "Flappy", windowStyle, 0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, nullptr, nullptr,
                               _hinstance, this);
    if (!hwnd)
    {
        return false;
    }

    LPDIRECTDRAW ddraw;
    CHECK(DirectDrawCreate(nullptr, &ddraw, nullptr));
    CHECK(ddraw->QueryInterface(IID_IDirectDraw4, (void**)&_ddraw));
    ddraw->Release();

    /* Load palette */
    _paletteEntries = loadPalette("doge.pal");
    assert(_paletteEntries.size() == 256);

    /* Adjust palette so the first and last 10 use windows' system palette colors */
    for (int i = 0; i < 10; i++)
    {
        _paletteEntries[i].peFlags = PC_EXPLICIT;
        _paletteEntries[i].peRed = i;
        _paletteEntries[i].peGreen = _paletteEntries[i].peBlue = 0;

        _paletteEntries[i + 246].peFlags = PC_EXPLICIT;
        _paletteEntries[i + 246].peRed = i + 246;
        _paletteEntries[i + 246].peGreen = _paletteEntries[i + 246].peBlue = 0;
    }

    createSurfaces();

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
    freeSurfaces();

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
            _fullscreen = !_fullscreen;
            if (!_fullscreen)
            {
                CHECK(_ddraw->RestoreDisplayMode());
            }

            createSurfaces();
        }
        else if (wparam == VK_F6)
        {
            onZoom(true);
        }
        else if (wparam == VK_F7)
        {
            onZoom(false);
        }

        return 0;
    }
    return std::nullopt;
}

void App::onPaint(WPARAM wparam, LPARAM lparam)
{
}

void App::onZoom(bool zoomIn)
{
    if (_fullscreen)
    {
        return;
    }

    if (zoomIn)
    {
        _zoom++;
    }
    else
    {
        _zoom--;
    }

    if (_zoom < 1)
    {
        _zoom = 1;
    }
    else if (_zoom > 8)
    {
        _zoom = 8;
    }

    RECT rc;
    rc.left = 0;
    rc.right = BASE_WIDTH * _zoom;
    rc.top = 0;
    rc.bottom = BASE_HEIGHT * _zoom;
    AdjustWindowRect(&rc, WINDOWED_STYLE, FALSE);
    SetWindowPos(_hwnd, nullptr, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);
    createSurfaces();
}

static void blit8to8(void* dstPtr, int dstWidth, int dstHeight, int dstPitch, const void* srcPtr, int srcWidth, int srcHeight, int srcPitch,
                     const PALETTEENTRY* palette)
{
    if (!dstPtr || !srcPtr)
    {
        return;
    }
    if (dstWidth <= 0 || dstHeight <= 0 || srcWidth <= 0 || srcHeight <= 0)
    {
        return;
    }

    auto dst = static_cast<uint8_t*>(dstPtr);
    auto src = static_cast<const uint8_t*>(srcPtr);

    // Fast path: identical size
    if (dstWidth == srcWidth && dstHeight == srcHeight)
    {
        if (dstPitch == srcPitch)
        {
            memcpy(dst, src, dstHeight * dstPitch);
        }
        else
        {
            for (int y = 0; y < dstHeight; y++)
            {
                memcpy(dst, src, dstWidth);
                dst += dstPitch;
                src += srcPitch;
            }
        }
        return;
    }

    const Fixed16 xRatio = dstWidth > 1 ? Fixed16(srcWidth) / Fixed16(dstWidth) : 0;
    const Fixed16 yRatio = dstHeight > 1 ? Fixed16(srcHeight) / Fixed16(dstHeight) : 0;
    int srcY = 0;
    for (int dy = 0; dy < dstHeight; ++dy)
    {
        const uint8_t* srcRow = src + srcY * srcPitch;
        uint8_t* dstRow = dst + dy * dstPitch;

        Fixed16 sxAcc(0);
        for (int dx = 0; dx < dstWidth; ++dx)
        {
            int srcX = sxAcc.toInt();
            if (srcX >= srcWidth)
            {
                srcX = srcWidth - 1;
            }

            dstRow[dx] = srcRow[srcX];
            sxAcc += xRatio;
        }

        if (dstHeight != 1)
        {
            srcY = (Fixed16(dy) * xRatio).toInt();
        }
        if (srcY >= srcHeight)
        {
            srcY = srcHeight - 1;
        }
    }
}

static void blit8to16(void* dstPtr, int dstWidth, int dstHeight, int dstPitch, const void* srcPtr, int srcWidth, int srcHeight, int srcPitch,
                      const PALETTEENTRY* palette)
{
    panic("Not implemented yet");
}

static void blit8to24(void* dstPtr, int dstWidth, int dstHeight, int dstPitch, const void* srcPtr, int srcWidth, int srcHeight, int srcPitch,
                      const PALETTEENTRY* palette)
{
    panic("Not implemented yet");
}

void blit8to32(void* dstPtr, int dstWidth, int dstHeight, int dstPitch, const void* srcPtr, int srcWidth, int srcHeight, int srcPitch,
               const PALETTEENTRY* palette)
{
    if (!dstPtr || !srcPtr)
    {
        return;
    }
    if (dstWidth <= 0 || dstHeight <= 0 || srcWidth <= 0 || srcHeight <= 0)
    {
        return;
    }

    auto dst = static_cast<uint8_t*>(dstPtr);
    auto src = static_cast<const uint8_t*>(srcPtr);

    // Fast path: identical size
    if (dstWidth == srcWidth && dstHeight == srcHeight)
    {
        for (int y = 0; y < dstHeight; y++)
        {
            uint32_t* scanline = reinterpret_cast<uint32_t*>(dst);
            for (int x = 0; x < dstWidth; x++)
            {
                auto color = palette[*src++];
                *scanline++ = color.peBlue | (color.peGreen << 8) | (color.peRed << 16);
            }
            dst += dstPitch;
        }
        return;
    }

    const Fixed16 xRatio = dstWidth > 1 ? Fixed16(srcWidth) / Fixed16(dstWidth) : 0;
    const Fixed16 yRatio = dstHeight > 1 ? Fixed16(srcHeight) / Fixed16(dstHeight) : 0;
    int srcY = 0;
    for (int dy = 0; dy < dstHeight; ++dy)
    {
        const uint8_t* srcRow = src + srcY * srcPitch;
        uint32_t* dstRow = reinterpret_cast<uint32_t*>(dst + dy * dstPitch);

        Fixed16 sxAcc(0);
        for (int dx = 0; dx < dstWidth; ++dx)
        {
            int srcX = sxAcc.toInt();
            if (srcX >= srcWidth)
            {
                srcX = srcWidth - 1;
            }

            auto c = palette[srcRow[srcX]];
            dstRow[dx] = c.peBlue | (c.peGreen << 8) | (c.peRed << 16);
            sxAcc += xRatio;
        }

        if (dstHeight != 1)
        {
            srcY = (Fixed16(dy) * xRatio).toInt();
        }
        if (srcY >= srcHeight)
        {
            srcY = srcHeight - 1;
        }
    }
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
    if (!_fullscreen)
    {
        /* Recreate if primary surface's size changed */
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

        /* Associated with the main surface in fullscreen mode */
        if (_backSurf->IsLost())
        {
            CHECK(_backSurf->Restore());
        }
    }

    /* Render to framebuffer */
    assert(_framebuffer.ptr.size() == _background.ptr.size());
    memcpy(_framebuffer.ptr.data(), _background.ptr.data(), _background.w * _background.h);

    /* Stretch and render framebuffer to backsurface */
    backsurfSize = getSurfaceSize(_backSurf);

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    CHECK(_backSurf->Lock(nullptr, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, nullptr));
    switch (ddsd.ddpfPixelFormat.dwRGBBitCount)
    {
    case 8:
        blit8to8(ddsd.lpSurface, ddsd.dwWidth, ddsd.dwHeight, ddsd.lPitch, _framebuffer.ptr.data(), _framebuffer.w, _framebuffer.h, _framebuffer.w,
                 _paletteEntries.data());
        break;
    case 16:
        blit8to16(ddsd.lpSurface, ddsd.dwWidth, ddsd.dwHeight, ddsd.lPitch, _framebuffer.ptr.data(), _framebuffer.w, _framebuffer.h, _framebuffer.w,
                  _paletteEntries.data());
        break;
    case 24:
        blit8to24(ddsd.lpSurface, ddsd.dwWidth, ddsd.dwHeight, ddsd.lPitch, _framebuffer.ptr.data(), _framebuffer.w, _framebuffer.h, _framebuffer.w,
                  _paletteEntries.data());
        break;
    case 32:
        blit8to32(ddsd.lpSurface, ddsd.dwWidth, ddsd.dwHeight, ddsd.lPitch, _framebuffer.ptr.data(), _framebuffer.w, _framebuffer.h, _framebuffer.w,
                  _paletteEntries.data());
        break;
    default:
        panic("Unsupported pixel format");
    }
    CHECK(_backSurf->Unlock(nullptr));

    /* Debug text */
    stbsp_snprintf(debugText, sizeof(debugText), "fps=%d w=%d h=%d zoom=%d", _fps, backsurfSize.width, backsurfSize.height, _zoom);

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

    if (_fullscreen)
    {
        if (auto ret = _primarySurf->Flip(nullptr, DDFLIP_WAIT); FAILED(ret))
        {
            trace("Flip() failed: 0x%X %s", ret, hresult2str(ret));
        }
    }
    else
    {
        if (auto ret = _primarySurf->Blt(&rc, _backSurf, nullptr, DDBLT_WAIT, nullptr); FAILED(ret))
        {
            trace("Blt failed: 0x%X %s", ret, hresult2str(ret));
        }
    }

    LeaveCriticalSection(&_criticalSection);
}

Bitmap App::loadBitmap(const char* name)
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

    Bitmap result;
    result.w = width;
    result.h = height;
    result.ptr.insert(result.ptr.end(), width * height, '\0');
    if (fread(result.ptr.data(), 1, result.ptr.size(), f) != result.ptr.size())
    {
        panic("Read error from %s", name);
    }
    return result;
}

void App::createSurfaces()
{
    trace("Creating surfaces");
    freeSurfaces();

    auto windowStyle = _fullscreen ? FULLSCREEN_STYLE : WINDOWED_STYLE;
    SetWindowLong(_hwnd, GWL_STYLE, windowStyle);

    if (_fullscreen)
    {
        SetWindowPos(_hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);

        CHECK(_ddraw->SetCooperativeLevel(_hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT));
        CHECK(_ddraw->SetDisplayMode(BASE_WIDTH * 2, BASE_HEIGHT * 2, 8, 0, 0));

        /* Create primary surface */
        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_BACKBUFFERCOUNT;
        ddsd.dwBackBufferCount = 1;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_COMPLEX | DDSCAPS_FLIP;
        CHECK(_ddraw->CreateSurface(&ddsd, &_primarySurf, nullptr));
    }
    else
    {
        CHECK(_ddraw->SetCooperativeLevel(_hwnd, DDSCL_NORMAL));

        SetWindowPos(_hwnd, nullptr, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOMOVE | SWP_FRAMECHANGED);

        RECT rcWindow;
        rcWindow.left = 0;
        rcWindow.top = 0;
        rcWindow.right = BASE_WIDTH * _zoom;
        rcWindow.bottom = BASE_HEIGHT * _zoom;
        AdjustWindowRect(&rcWindow, windowStyle, FALSE);
        SetWindowPos(_hwnd, nullptr, 0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, SWP_NOZORDER | SWP_NOMOVE);

        /* Create primary surface */
        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        CHECK(_ddraw->CreateSurface(&ddsd, &_primarySurf, nullptr));
    }

    ShowWindow(_hwnd, SW_SHOWDEFAULT);
    UpdateWindow(_hwnd);

    /* Set palette */
    DDPIXELFORMAT pf;
    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    CHECK(_primarySurf->GetPixelFormat(&pf));
    if (pf.dwFlags & DDPF_PALETTEINDEXED8)
    {
        CHECK(_ddraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE, _paletteEntries.data(), &_palette, nullptr));
        CHECK(_primarySurf->SetPalette(_palette));
    }
    else if ((pf.dwFlags & DDPF_RGB) == 0)
    {
        panic("Unsupported pixel format");
    }

    /* Create backsurface */
    if (_fullscreen)
    {
        DDSCAPS2 caps;
        memset(&caps, 0, sizeof(caps));
        caps.dwCaps = DDSCAPS_BACKBUFFER;
        CHECK(_primarySurf->GetAttachedSurface(&caps, &_backSurf));
    }
    else
    {
        RECT cltRect;
        GetClientRect(_hwnd, &cltRect);

        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.dwWidth = cltRect.right - cltRect.left;
        ddsd.dwHeight = cltRect.bottom - cltRect.top;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        CHECK(_ddraw->CreateSurface(&ddsd, &_backSurf, nullptr));

        LPDIRECTDRAWCLIPPER clipper;
        CHECK(_ddraw->CreateClipper(0, &clipper, nullptr));
        CHECK(clipper->SetHWnd(0, _hwnd));
        CHECK(_primarySurf->SetClipper(clipper));
        clipper->Release();
    }

    /* Load bitmaps */
    trace("Loading swatch.dat");
    _background = loadBitmap("swatch.dat");
    trace("Loading tiles1.dat");
    _tiles1 = loadBitmap("tiles1.dat");

    trace("Done creating surfaces");
}

void App::freeSurfaces()
{
    if (_backSurf)
    {
        _backSurf->Release();
        _backSurf = nullptr;
    }

    if (_primarySurf)
    {
        _primarySurf->Release();
        _primarySurf = nullptr;
    }

    if (_palette)
    {
        _palette->Release();
        _palette = nullptr;
    }
}

std::unique_ptr<IApp> makeApp(HINSTANCE hInstance)
{
    return std::make_unique<App>(hInstance);
}

