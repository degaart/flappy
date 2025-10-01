#include "util.hpp"

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <mmeapi.h>
#include <dsound.h>

#define STB_VORBIS_HEADER_ONLY
#include "../stb_vorbis.c"

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
    LPDIRECTSOUND _dsound;
    std::vector<LPDIRECTSOUNDBUFFER> _sndBuffers;

    int _fps;

    static constexpr auto GAME_WIDTH = 320;
    static constexpr auto GAME_HEIGHT = 240;
    static constexpr auto WINDOWSTYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    void createSurfaces();
    void freeSurfaces();
    std::optional<LRESULT> onEvent(HWND, UINT, WPARAM, LPARAM) override;
    Bitmap loadBitmap(const char* filename);
    void update(double dT);
    void render();
    static int getBPP(DDPIXELFORMAT* pf);
    static uint32_t makeRGB(uint8_t r, uint8_t g, uint8_t b, const DDPIXELFORMAT* ddpf);
};

App::App(HINSTANCE hInstance)
    : IApp(hInstance),
    _ddraw(nullptr), _primarySurf(nullptr),
    _backSurf(nullptr),
    _fullscreen(false), _zoom(2), _active(false),
    _dsound(nullptr)
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

    trace("Palette entry 255: %d,%d,%d", _paletteEntries[255].peRed, _paletteEntries[255].peGreen, _paletteEntries[255].peBlue);

    createSurfaces();

    CHECK(DirectSoundCreate(nullptr, &_dsound, nullptr));
    CHECK(_dsound->SetCooperativeLevel(_hwnd, DSSCL_NORMAL));

    static const char* SFX_FILES[] = {
        "collision.ogg",
        "jump1.ogg",
        "jump2.ogg",
        "jump3.ogg",
        "jump4.ogg",
        "jump5.ogg",
    };
    for (const auto& sfxFile: SFX_FILES)
    {
        trace("Loading %s", sfxFile);
        int channels, sampleRate;
        int16_t* samples;
        auto sampleCount = stb_vorbis_decode_filename(sfxFile, &channels, &sampleRate, &samples);
        if (sampleCount == -1)
        {
            panic("Failed to decode %s", sfxFile);
        }
        if (sampleRate != 22050)
        {
            panic("Unsupported samplerate for %s: %d", sfxFile, sampleRate);
        }
        else if (channels != 1)
        {
            panic("Unsupported number of channels for %s: %d", sfxFile, channels);
        }

        WAVEFORMATEX wfe;
        memset(&wfe, 0, sizeof(wfe));
        wfe.wFormatTag = WAVE_FORMAT_PCM;
        wfe.nChannels = 1;
        wfe.nSamplesPerSec = 22050;
        wfe.wBitsPerSample = 16;
        wfe.nBlockAlign = wfe.nChannels * (wfe.wBitsPerSample / 8); /* nChannels * nBytesPerSample */
        wfe.nAvgBytesPerSec = wfe.nSamplesPerSec * wfe.nBlockAlign;
        wfe.cbSize = 0;

        DSBUFFERDESC dsbd;
        memset(&dsbd, 0, sizeof(dsbd));
        dsbd.dwSize = sizeof(dsbd);
        dsbd.dwFlags = DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLFREQUENCY | DSBCAPS_STATIC | DSBCAPS_LOCSOFTWARE;
        dsbd.dwBufferBytes = sampleCount * sizeof(int8_t); /* 2 secs at sample rate 11025 */
        dsbd.lpwfxFormat = &wfe;

        LPDIRECTSOUNDBUFFER sndBuf;
        CHECK(_dsound->CreateSoundBuffer(&dsbd, &sndBuf, nullptr));

        void* audioPtr1;
        unsigned long audioBytes1;
        void* audioPtr2;
        unsigned long audioBytes2;
        CHECK(sndBuf->Lock(0, dsbd.dwBufferBytes, &audioPtr1, &audioBytes1, &audioPtr2, &audioBytes2, DSBLOCK_ENTIREBUFFER));
        memcpy(audioPtr1, samples, audioBytes1);
        CHECK(sndBuf->Unlock(audioPtr1, audioBytes1, audioPtr2, audioBytes2));
        free(samples);

        _sndBuffers.push_back(sndBuf);
    }

    _active = true;

    UpdateWindow(hwnd);
    ShowWindow(hwnd, SW_SHOWDEFAULT);
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

        if (_active)
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
    for (auto sndBuf : _sndBuffers)
    {
        sndBuf->Release();
    }

    _dsound->Release();
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
        CHECK(_ddraw->RestoreDisplayMode());
        CHECK(_ddraw->SetCooperativeLevel(_hwnd, DDSCL_NORMAL));

        RECT windowRect;
        windowRect.left = windowRect.top = 0;
        windowRect.right = GAME_WIDTH * _zoom;
        windowRect.bottom = GAME_HEIGHT * _zoom;
        AdjustWindowRect(&windowRect, WINDOWSTYLE, FALSE);
        SetWindowPos(_hwnd, NULL, 0, 0, windowRect.right - windowRect.left, windowRect.bottom - windowRect.top, SWP_NOMOVE | SWP_NOZORDER | SWP_NOREDRAW);

        PostMessage(HWND_BROADCAST, WM_PAINT, 0, 0);
        InvalidateRect(NULL, NULL, TRUE);
        UpdateWindow(GetDesktopWindow());

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
    if (getBPP(&pf) == 8)
    {
        LPDIRECTDRAWPALETTE palette;
        CHECK(_ddraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE, _paletteEntries.data(), &palette, nullptr));
        CHECK(_primarySurf->SetPalette(palette));
        palette->Release();
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

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    CHECK(_backSurf->GetSurfaceDesc(&ddsd));
    trace("Backsurf size: %dx%d", ddsd.dwWidth, ddsd.dwHeight);
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
    case WM_KEYUP:
        switch (wparam)
        {
        case VK_ESCAPE:
            PostMessageA(_hwnd, WM_CLOSE, 0, 0);
            break;
        case VK_F5:
            _fullscreen = !_fullscreen;
            if (!_fullscreen)
            {
                CHECK(_ddraw->RestoreDisplayMode());
            }
            createSurfaces();
            break;
        case VK_F6:
            if (_zoom > 1)
            {
                _zoom--;
            }
            createSurfaces();
            break;
        case VK_F7:
            if (_zoom < 8)
            {
                _zoom++;
            }
            createSurfaces();
            break;
        case VK_F3:
            {
                static int currentSound = 0;
                CHECK(_sndBuffers[currentSound]->Play(0, 0, 0));
                currentSound = (currentSound+1) % _sndBuffers.size();
                break;
            }
        }
        break;
    case WM_MOUSEWHEEL:
        if (int zDelta = static_cast<short>(HIWORD(wparam)) / WHEEL_DELTA; zDelta > 0)
        {
            _zoom++;
            if (_zoom >= 8)
            {
                _zoom = 8;
            }
        }
        else
        {
            _zoom--;
            if (_zoom < 1)
            {
                _zoom = 1;
            }
        }
        createSurfaces();
        break;
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
    CHECK(result.surf->Lock(nullptr, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, nullptr));

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
    case 16:
    case 24:
    case 32:
        for (int y = 0; y < height; y++)
        {
            const uint8_t* srcPtr = rawData.data() + (y * width);
            uint32_t* dstPtr = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(ddsd.lpSurface) + (y * ddsd.lPitch));
            for (int x = 0; x < width; x++)
            {
                PALETTEENTRY c;
                switch (*srcPtr)
                {
                case 0:
                    c.peRed = c.peGreen = c.peBlue = 0;
                    break;
                case 255:
                    c.peRed = c.peGreen = c.peBlue = 255;
                    break;
                default:
                    c = _paletteEntries[*srcPtr];
                    break;
                }
                *dstPtr = makeRGB(c.peRed, c.peGreen, c.peBlue, &pf);
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

void App::update(double dT)
{
}

void App::render()
{
    char debugText[255] = "";

    if (_backSurf->IsLost())
    {
        CHECK(_backSurf->Restore());
    }

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    CHECK(_backSurf->GetSurfaceDesc(&ddsd));
    int backsurfWidth = ddsd.dwWidth;
    int backsurfHeight = ddsd.dwHeight;

    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);

    int bpp = getBPP(&ddsd.ddpfPixelFormat);
    switch (bpp)
    {
    case 8:
        fx.dwFillColor = 111;
        break;
    case 32:
        fx.dwFillColor = makeRGB(102, 204, 255, &ddsd.ddpfPixelFormat);
        break;
    default:
        panic("Unsupported pixel format");
    }

    CHECK(_backSurf->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fx));

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

    DDCOLORKEY cckey;
    switch (bpp)
    {
    case 8:
        cckey.dwColorSpaceLowValue = 195;
        cckey.dwColorSpaceHighValue = 195;
        break;
    case 16:
    case 24:
    case 32:
        cckey.dwColorSpaceLowValue = makeRGB(255, 0, 255, &ddsd.ddpfPixelFormat);
        cckey.dwColorSpaceHighValue = cckey.dwColorSpaceLowValue;
        break;
    default:
        panic("Unsupported pixel format");
        break;
    }
    CHECK(_tiles1.surf->SetColorKey(DDCKEY_SRCBLT, &cckey));
    CHECK(_backSurf->BltFast(0, 0, _tiles1.surf, &srcRect, DDBLTFAST_WAIT | DDBLTFAST_SRCCOLORKEY));

    if (_primarySurf->IsLost())
    {
        _primarySurf->Restore();
    }

    stbsp_snprintf(debugText, sizeof(debugText), "fps=%d", _fps);
    HDC hdc;
    CHECK(_backSurf->GetDC(&hdc));
    SetTextColor(hdc, RGB(255, 0, 0));
    TextOut(hdc, 0, 0, debugText, lstrlen(debugText));
    _backSurf->ReleaseDC(hdc);

    if (_fullscreen)
    {
        REPORT(_primarySurf->Flip(nullptr, DDFLIP_WAIT));
    }
    else
    {
        POINT origin;
        origin.x = 0;
        origin.y = 0;
        ClientToScreen(_hwnd, &origin);

        GetClientRect(_hwnd, &dstRect);
        OffsetRect(&dstRect, origin.x, origin.y);

        srcRect.left = 0;
        srcRect.top = 0;
        srcRect.right = backsurfWidth;
        srcRect.bottom = backsurfHeight;
        REPORT(_primarySurf->Blt(&dstRect, _backSurf, &srcRect, DDBLT_WAIT, nullptr));
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

uint32_t App::makeRGB(uint8_t r, uint8_t g, uint8_t b, const DDPIXELFORMAT* ddpf)
{
    // Normalize to the mask size
    auto redMask = ddpf->dwRBitMask;
    auto greenMask = ddpf->dwGBitMask;
    auto blueMask = ddpf->dwBBitMask;

    // Find bit shifts
    int rShift = 0;
    while (((redMask >> rShift) & 1) == 0 && rShift < 32)
    {
        rShift++;
    }

    int gShift = 0;
    while (((greenMask >> gShift) & 1) == 0 && gShift < 32)
    {
        gShift++;
    }

    int bShift = 0;
    while (((blueMask >> bShift) & 1) == 0 && bShift < 32)
    {
        bShift++;
    }

    // Scale 0–255 channel values down to mask size
    int rBits = 0, gBits = 0, bBits = 0;
    while ((1U << rBits) - 1 < (redMask >> rShift))
    {
        rBits++;
    }

    while ((1U << gBits) - 1 < (greenMask >> gShift))
    {
        gBits++;
    }

    while ((1U << bBits) - 1 < (blueMask >> bShift))
    {
        bBits++;
    }

    auto rVal = ((r * ((1 << rBits) - 1)) / 255) << rShift;
    auto gVal = ((g * ((1 << gBits) - 1)) / 255) << gShift;
    auto bVal = ((b * ((1 << bBits) - 1)) / 255) << bShift;

    return (rVal & redMask) | (gVal & greenMask) | (bVal & blueMask);
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

