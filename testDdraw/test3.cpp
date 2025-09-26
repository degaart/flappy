#include "util.hpp"

#include <assert.h>

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
    LPDIRECTDRAWSURFACE4 _tiles;
    bool _fullscreen;
    int _zoom;
    std::vector<PALETTEENTRY> _paletteEntries;
    bool _active;

    static constexpr auto GAME_WIDTH = 320;
    static constexpr auto GAME_HEIGHT = 240;
    static constexpr auto WINDOWSTYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    void createSurfaces();
    void freeSurfaces();
    std::optional<LRESULT> onEvent(HWND, UINT, WPARAM, LPARAM) override;
};

App::App(HINSTANCE hInstance)
    : IApp(hInstance),
      _ddraw(nullptr),
      _primarySurf(nullptr),
      _backSurf(nullptr),
      _tiles(nullptr),
      _fullscreen(false),
      _zoom(1),
      _active(false)
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

    RECT rcWindow;
    rcWindow.left = 0;
    rcWindow.top = 0;
    rcWindow.right = GAME_WIDTH * _zoom;
    rcWindow.bottom = GAME_HEIGHT * _zoom;
    AdjustWindowRect(&rcWindow, WINDOWSTYLE, FALSE);

    HWND hwnd = CreateWindowEx(0, "MainWin", "Flappy", WINDOWSTYLE, 0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, nullptr, nullptr,
                               _hinstance, this);
    if (!hwnd)
    {
        return false;
    }

    LPDIRECTDRAW ddraw;
    CHECK(DirectDrawCreate(nullptr, &ddraw, nullptr));
    CHECK(ddraw->QueryInterface(IID_IDirectDraw4, (void**)&_ddraw));
    ddraw->Release();
    ddraw = nullptr;

    _paletteEntries = loadPalette("doge.pal");
    assert(_paletteEntries.size() == 256);
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
    _active = true;
    return false;
}

int App::run()
{
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
}

void App::createSurfaces()
{
    freeSurfaces();
    if (_fullscreen)
    {
        CHECK(_ddraw->SetCooperativeLevel(_hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_ALLOWMODEX));
        CHECK(_ddraw->SetDisplayMode(GAME_WIDTH, GAME_HEIGHT, 8, 0, 0));

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
        panic("Not implemented yet");
    }

    DDPIXELFORMAT pf;
    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    CHECK(_primarySurf->GetPixelFormat(&pf));
    if (pf.dwFlags & DDPF_PALETTEINDEXED8)
    {
        LPDIRECTDRAWPALETTE palette;
        CHECK(_ddraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE, _paletteEntries.data(), &palette, nullptr));
        CHECK(_primarySurf->SetPalette(palette));
        palette->Release();
    }
    else if ((pf.dwFlags & DDPF_RGB) == 0)
    {
        panic("Unsupported pixel format");
    }






}

std::optional<LRESULT> App::onEvent(HWND, UINT, WPARAM, LPARAM)
{
    return std::nullopt;
}




