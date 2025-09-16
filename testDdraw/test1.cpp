#include "util.hpp"

#include <assert.h>
#include <ddraw.h>
#include <shlwapi.h>
#include <stdint.h>
#include <stdio.h>
#include <windows.h>

/*
 * Needed operations:
 *  - render rectangle
 *  - load image into surface
 *  - blit (with transparency)
 *  - clear screen?
 */

class TestApp : public IApp
{
public:
    explicit TestApp(HINSTANCE hInstance);
    bool init() override;
    int run() override;
    void cleanup() override;

private:
    HWND _hwnd;
    LPDIRECTDRAW4 _ddraw;
    LPDIRECTDRAWSURFACE4 _primarySurface;
    LPDIRECTDRAWSURFACE4 _backbuffer;
    LPDIRECTDRAWCLIPPER _clipper;

    std::optional<LRESULT> onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;
    void onKeyUp(int keyCode, int scancode);
    void onDestroy();
    void render();
};

TestApp::TestApp(HINSTANCE hInstance)
    : IApp(hInstance)
{
}

bool TestApp::init()
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.hInstance = _hinstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpfnWndProc = TestApp::windowProc;
    wc.lpszClassName = "MainWin";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassEx(&wc))
    {
        return false;
    }

    HWND hwnd = CreateWindowEx(0, "MainWin", "Zinzolu", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, _hinstance, this);
    if (!hwnd)
    {
        return false;
    }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    LPDIRECTDRAW ddraw;
    CHECK(DirectDrawCreate(nullptr, &ddraw, nullptr));
    CHECK(ddraw->QueryInterface(IID_IDirectDraw4, (void**)&_ddraw));
    ddraw->Release();

    CHECK(_ddraw->SetCooperativeLevel(hwnd, DDSCL_NORMAL));

    DDSURFACEDESC2 ddsd2;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    CHECK(_ddraw->GetDisplayMode(&ddsd2));

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

    CHECK(_ddraw->CreateSurface(&primarySD, &_primarySurface, nullptr));
    CHECK(_ddraw->CreateClipper(0, &_clipper, nullptr));

    assert(hwnd != nullptr);
    CHECK(_clipper->SetHWnd(0, hwnd));

    // if (FAILED(_primarySurface->SetClipper(_clipper)))
    //{
    //     panic("SetClipper() failed");
    // }
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
    CHECK(_ddraw->CreateSurface(&bbsd, &_backbuffer, nullptr));

    return true;
}

int TestApp::run()
{
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        render();
    }
    return msg.wParam;
}

void TestApp::cleanup()
{
    _clipper->Release();
    _clipper = nullptr;

    _backbuffer->Release();
    _backbuffer = nullptr;

    _primarySurface->Release();
    _primarySurface = nullptr;

    _ddraw->Release();
    _ddraw = nullptr;
}

void TestApp::render()
{
    if (_backbuffer == nullptr)
    {
        return;
    }

    DDSURFACEDESC2 ddsd2;
    memset(&ddsd2, 0, sizeof(ddsd2));
    ddsd2.dwSize = sizeof(ddsd2);
    CHECK(_backbuffer->Lock(nullptr, &ddsd2, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, nullptr));

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

    CHECK(_backbuffer->Unlock(nullptr));

    POINT origin;
    origin.x = origin.y = 0;
    ClientToScreen(_hwnd, &origin);

    RECT rc;
    GetClientRect(_hwnd, &rc);
    OffsetRect(&rc, origin.x, origin.y);

    /* Clear screen */
    HRESULT hResult;

    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = 0xFF;
    while (true)
    {
        hResult = _primarySurface->Blt(&rc, nullptr, nullptr, DDBLT_COLORFILL, &fx);
        if (hResult == DDERR_SURFACELOST)
        {
            _primarySurface->Restore();
        }
        else if (FAILED(hResult))
        {
            panic("Blt() failed: 0x%lX", hResult);
        }
        else
        {
            break;
        }
    }

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
            panic("Blt() failed: 0x%lX", hResult);
        }
        else
        {
            break;
        }
    }
}

std::optional<LRESULT> TestApp::onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
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
    return std::nullopt;
}

void TestApp::onKeyUp(int keyCode, int scancode)
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

void TestApp::onDestroy()
{
    PostQuitMessage(0);
}

std::unique_ptr<IApp> makeApp(HINSTANCE hInstance)
{
    return std::make_unique<TestApp>(hInstance);
}


