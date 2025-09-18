#include "util.hpp"

#include <assert.h>
#include <ddraw.h>

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
    void onPaint(WPARAM,LPARAM);
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

    RECT winRect;
    winRect.left = winRect.top = 0;
    winRect.right = 640;
    winRect.bottom = 480;
    AdjustWindowRect(&winRect, windowStyle, FALSE);

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
    MSG msg;
    //while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
    //{
    //    if (msg.message == WM_QUIT)
    //    {
    //        return msg.wParam;;
    //    }
    //    TranslateMessage(&msg);
    //    DispatchMessage(&msg);
    //}
    //WaitMessage();
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        //render();
    }
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

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    CHECK(_backSurf->GetSurfaceDesc(&ddsd));
    if (ddsd.dwWidth != rc.right - rc.left || ddsd.dwHeight != rc.bottom - rc.top)
    {
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.dwWidth = rc.right - rc.left;
        ddsd.dwHeight = rc.bottom - rc.top;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        CHECK(_ddraw->CreateSurface(&ddsd, &_backSurf, nullptr));
    }

    DDBLTFX fx;
    fx.dwSize = sizeof(fx);
    fx.dwFillColor = 0xFFFFFFFF;

    CHECK(_backSurf->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fx));
    CHECK(_primarySurf->Blt(&rc, _backSurf, nullptr, DDBLT_WAIT, nullptr));
}

std::unique_ptr<IApp> makeApp(HINSTANCE hInstance)
{
    return std::make_unique<App>(hInstance);
}

