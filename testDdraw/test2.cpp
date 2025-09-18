#include "util.hpp"

#include <assert.h>
#include <ddraw.h>
#include <stdint.h>

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

    std::optional<LRESULT> onEvent(HWND, UINT, WPARAM, LPARAM) override;
    void onPaint(WPARAM, LPARAM);
    void render();
};

App::App(HINSTANCE hInstance)
    : IApp(hInstance), _running(false)
{
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
    HWND hwnd = CreateWindowEx(0, "MainWin", "Zinzolu", windowStyle, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, _hinstance, this);
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

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    _running = true;
    return true;
}

int App::run()
{
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
        // render();
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
        return 0;
    case WM_PAINT:
        onPaint(wparam, lparam);
        return 0;
    }
    return std::nullopt;
}

void App::onPaint(WPARAM wparam, LPARAM lparam)
{
    if (_running)
    {
        assert(_primarySurf != nullptr);
        assert(_backSurf != nullptr);
        render();
    }
}

void App::render()
{
    POINT origin;
    origin.x = origin.y = 0;
    ClientToScreen(_hwnd, &origin);

    RECT rc;
    GetClientRect(_hwnd, &rc);
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
    if (ddsd.dwWidth != rc.right - rc.left || ddsd.dwHeight != rc.bottom - rc.top)
    {
        _ddraw->Release();

        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.dwWidth = rc.right - rc.left;
        ddsd.dwHeight = rc.bottom - rc.top;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        CHECK(_ddraw->CreateSurface(&ddsd, &_backSurf, nullptr));
    }

    /* Clear backbuffer */
    DDBLTFX fx;
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = 0xFFFFFFFF;
    RETRY(_backSurf->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fx));

    /* Render pattern */
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    RETRY(_backSurf->Lock(nullptr, &ddsd, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, nullptr));

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint32_t* ptr = reinterpret_cast<uint32_t*>(ddsd.lpSurface);
    for (int y = 0; y < ddsd.dwHeight; y++)
    {
        for (int x = 0; x < ddsd.dwWidth; x++)
        {
            *ptr = r | (g << 8) | (b << 16) | (0xFF << 24);
            ptr++;
            r++;
            b++;
        }
        g++;
        ptr = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(ptr) + ddsd.lPitch);
    }
    CHECK(_backSurf->Unlock(nullptr));

    /* Blit backbuffer to main surface */
    RETRY(_primarySurf->Blt(&rc, _backSurf, nullptr, DDBLT_WAIT, nullptr));
}

std::unique_ptr<IApp> makeApp(HINSTANCE hInstance)
{
    return std::make_unique<App>(hInstance);
}

