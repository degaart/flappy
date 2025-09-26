#include "util.hpp"

#include <assert.h>
#include <stdint.h>

struct Bitmap
{
    int w, h;
    LPDIRECTDRAWSURFACE4 surf;

    Bitmap();
    Bitmap(const Bitmap&) = delete;
    Bitmap(Bitmap&&);
    Bitmap& operator=(const Bitmap&) = delete;
    Bitmap& operator=(Bitmap&&);
    ~Bitmap();
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
    bool _fullscreen;
    int _zoom;
    std::vector<PALETTEENTRY> _paletteEntries;
    bool _active;
    Bitmap _tiles1;

    static constexpr auto GAME_WIDTH = 320;
    static constexpr auto GAME_HEIGHT = 240;
    static constexpr auto WINDOWSTYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    void createSurfaces();
    void freeSurfaces();
    std::optional<LRESULT> onEvent(HWND, UINT, WPARAM, LPARAM) override;
    Bitmap loadBitmap(const char* filename);
    void render();
    static int getBPP(DDPIXELFORMAT* pf);
    static uint32_t makeRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
    static uint32_t makeRGB(uint8_t r, uint8_t g, uint8_t b);
};

App::App(HINSTANCE hInstance)
    : IApp(hInstance),
      _ddraw(nullptr),
      _primarySurf(nullptr),
      _backSurf(nullptr),
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

    UpdateWindow(hwnd);
    ShowWindow(hwnd, SW_SHOWDEFAULT);
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

        if (_active)
        {
            render();
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
        CHECK(_ddraw->SetCooperativeLevel(_hwnd, DDSCL_NORMAL));

        RECT rcWindow;
        GetClientRect(_hwnd, &rcWindow);

        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS;
        ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;
        CHECK(_ddraw->CreateSurface(&ddsd, &_primarySurf, nullptr));
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

    if (_fullscreen)
    {
        DDSCAPS2 caps;
        memset(&caps, 0, sizeof(caps));
        caps.dwCaps = DDSCAPS_BACKBUFFER;
        CHECK(_primarySurf->GetAttachedSurface(&caps, &_backSurf));
    }
    else
    {
        //RECT cltRect;
        //GetClientRect(_hwnd, &cltRect);

        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(ddsd));
        ddsd.dwSize = sizeof(ddsd);
        ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
        ddsd.dwWidth = GAME_WIDTH;
        ddsd.dwHeight = GAME_HEIGHT;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        CHECK(_ddraw->CreateSurface(&ddsd, &_backSurf, nullptr));

        LPDIRECTDRAWCLIPPER clipper;
        CHECK(_ddraw->CreateClipper(0, &clipper, nullptr));
        CHECK(clipper->SetHWnd(0, _hwnd));
        CHECK(_primarySurf->SetClipper(clipper));
        clipper->Release();
    }

    _tiles1 = loadBitmap("tiles1.dat");
}

void App::freeSurfaces()
{
    if (_tiles1.surf)
    {
        _tiles1.surf->Release();
        _tiles1.surf = nullptr;
    }
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
}

std::optional<LRESULT> App::onEvent(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_ACTIVATE:
        _active = (LOWORD(wparam) != 0);
        return 0;
    }
    return std::nullopt;
}

Bitmap App::loadBitmap(const char* filename)
{
    FILE* f = fopen(filename, "rb");
    if (!f)
    {
        panic("Failed to open file %s", filename);
    }

    char debugInfo[16];
    if (fread(debugInfo, sizeof(debugInfo), 1, f) != 1)
    {
        panic("Read error from %s", filename);
    }
    trace("%s: %s", filename, debugInfo);

    int width;
    if (fread(&width, sizeof(width), 1, f) != 1)
    {
        panic("Read error from %s", filename);
    }

    int height;
    if (fread(&height, sizeof(height), 1, f) != 1)
    {
        panic("Read error from %s", filename);
    }
    trace("%s: %dx%d", filename, width, height);

    std::vector<uint8_t> rawData(width * height, 0);
    if (fread(rawData.data(), 1, rawData.size(), f) != rawData.size())
    {
        panic("Read error from %s", filename);
    }

    Bitmap result;
    result.w = width;
    result.h = height;

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    CHECK(_ddraw->CreateSurface(&ddsd, &result.surf, nullptr));

    DDPIXELFORMAT pf;
    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    CHECK(result.surf->GetPixelFormat(&pf));

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    CHECK(result.surf->Lock(nullptr, &ddsd, DDLOCK_SURFACEMEMORYPTR|DDLOCK_WAIT, nullptr));

    switch (getBPP(&pf))
    {
    case 8:
        for (int y = 0; y < height; y++)
        {
            const uint8_t* srcPtr = rawData.data() + (y * width);
            uint8_t* dstPtr = static_cast<uint8_t*>(ddsd.lpSurface) + (y * ddsd.lPitch);
            memcpy(dstPtr, srcPtr, width);
        }
        break;
    case 32:
        for (int y = 0; y < height; y++)
        {
            const uint8_t* srcPtr = rawData.data() + (y * width);
            uint32_t* dstPtr = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(ddsd.lpSurface) + (y * ddsd.lPitch));
            for (int x = 0; x < width; x++)
            {
                auto c = _paletteEntries[*srcPtr];
                *dstPtr = makeRGB(c.peRed, c.peGreen, c.peBlue);
                srcPtr++;
                dstPtr++;
            }
        }
        break;
    default:
        panic("Unsupported pixel format");
    }

    CHECK(result.surf->Unlock(nullptr));
    return result;
}

void App::render()
{
    if (_backSurf->IsLost())
    {
        CHECK(_backSurf->Restore());
    }

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    CHECK(_backSurf->GetSurfaceDesc(&ddsd));

    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);
    switch (getBPP(&ddsd.ddpfPixelFormat))
    {
        case 8:
            fx.dwFillColor = 111;
            break;
        case 32:
            fx.dwFillColor = makeRGB(102, 204, 255);
            break;
        default:
            panic("Unsupported pixel format");
    }
    
    CHECK(_backSurf->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL, &fx));

    if (_tiles1.surf->IsLost())
    {
        _tiles1.surf->Restore();
    }

    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = _tiles1.w;
    srcRect.bottom = _tiles1.h;

    RECT dstRect;
    dstRect.left = 0;
    dstRect.top = 0;
    dstRect.right = _tiles1.w;
    dstRect.bottom = _tiles1.h;
    CHECK(_backSurf->Blt(&dstRect, _tiles1.surf, &srcRect, DDBLT_WAIT, nullptr));

    if (_primarySurf->IsLost())
    {
        _primarySurf->Restore();
    }

    if (_fullscreen)
    {
        CHECK(_primarySurf->Flip(nullptr, DDFLIP_WAIT));
    }
    else
    {
        POINT origin;
        origin.x = 0;
        origin.y = 0;
        ClientToScreen(_hwnd, &origin);

        GetClientRect(_hwnd, &dstRect);
        OffsetRect(&dstRect, origin.x, origin.y);
        CHECK(_primarySurf->Blt(&dstRect, _backSurf, &srcRect, DDBLT_WAIT, nullptr));
    }
}

int App::getBPP(DDPIXELFORMAT* pf)
{
    if (pf->dwFlags & DDPF_PALETTEINDEXED8)
    {
        return 8;
    }
    else if (pf->dwFlags & DDPF_RGB)
    {
        return pf->dwRGBBitCount;
    }
    else
    {
        panic("Unsupported pixel format");
        return 0;
    }
}

uint32_t App::makeRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return makeRGBA(r, g, b, 0xFF);
}

uint32_t App::makeRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
    return b | (g << 8) | (r << 16) | (a << 24);
}

Bitmap::Bitmap()
    : w(0), h(0), surf(nullptr)
{
}

Bitmap::Bitmap(Bitmap&& other)
    : w(other.w), h(other.h), surf(nullptr)
{
    std::swap(surf, other.surf);
}

Bitmap& Bitmap::operator=(Bitmap&& other)
{
    if (this != &other)
    {
        if (surf)
        {
            surf->Release();
            surf = nullptr;
        }
        w = other.w;
        h = other.h;
        std::swap(surf, other.surf);
    }
    return *this;
}

Bitmap::~Bitmap()
{
    if (surf)
    {
        surf->Release();
    }
}

std::unique_ptr<IApp> makeApp(HINSTANCE hInstance)
{
    return std::make_unique<App>(hInstance);
}

