#include "util.hpp"

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

    std::optional<LRESULT> onEvent(HWND, UINT, WPARAM, LPARAM) override;
};

App::App(HINSTANCE hInstance)
    : IApp(hInstance)
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
    CHECK(ddraw->SetCooperativeLevel(hwnd, DDSCL_NORMAL));

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS;
    ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
    CHECK(_ddraw->CreateSurface(&ddsd, &_primarySurf, nullptr));

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = 640;
    ddsd.dwHeight = 480;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    CHECK(_ddraw->CreateSurface(&ddsd, &_backSurf, nullptr));

    CHECK(_ddraw->CreateClipper(0, &_clipper, nullptr));
    CHECK(_clipper->SetHWnd(0, hwnd));
    CHECK(_primarySurf->SetClipper(_clipper));

    return true;
}

int App::run()
{
    while (true)
    {
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
            {
                return msg.wParam;;
            }
        }
        WaitMessage();
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
        DestroyWindow(hwnd);
        return 0;
    case WM_KEYUP:
        if (wparam == VK_ESCAPE)
        {
            SendMessage(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        }
        return 0;
    }
    return std::nullopt;
}

std::unique_ptr<IApp> makeApp(HINSTANCE hInstance)
{
    return std::make_unique<App>(hInstance);
}

