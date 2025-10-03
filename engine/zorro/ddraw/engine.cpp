#include "engine.hpp"

#include <shlwapi.h>
#include <zorro/IGame.hpp>
#include <zorro/stb_sprintf.h>
#include <zorro/util.hpp>
#include <cmath>

#define STB_VORBIS_HEADER_ONLY
#include <zorro/stb_vorbis.c>

using namespace zorro;

extern "C" const char* __asan_default_options()
{
    return "detect_leaks=0";
}

Engine::Engine(HINSTANCE hInstance)
    : _hInstance(hInstance),
      _zoom(1),
      _active(false),
      _fps(0),
      _fullscreen(false),
      _hwnd(nullptr),
      _ddraw(nullptr),
      _primarySurf(nullptr),
      _backSurf(nullptr),
      _dsound(nullptr)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    _hrtFreq = static_cast<double>(freq.QuadPart);
    memset(_paletteEntries, 0, sizeof(_paletteEntries));
    _pixelFormat.valid = false;
}

void Engine::reloadBitmap(Bitmap* bmp)
{
    trace("Loading %s", bmp->_filename.c_str());

    int width, height;
    std::vector<uint8_t> data = zorro::loadBmp(bmp->_filename.c_str(), &width, &height);

    DDSURFACEDESC2 ddsd;
    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    ddsd.dwFlags = DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT;
    ddsd.dwWidth = width;
    ddsd.dwHeight = height;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;

    LPDIRECTDRAWSURFACE4 surf;
    CHECK(_ddraw->CreateSurface(&ddsd, &surf, nullptr));

    DDPIXELFORMAT pf;
    memset(&pf, 0, sizeof(pf));
    pf.dwSize = sizeof(pf);
    CHECK(surf->GetPixelFormat(&pf));

    memset(&ddsd, 0, sizeof(ddsd));
    ddsd.dwSize = sizeof(ddsd);
    CHECK(surf->Lock(nullptr, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, nullptr));

    int bpp = getBPP(&pf);
    switch (bpp)
    {
    case 8:
        if (ddsd.lPitch == width)
        {
            memcpy(ddsd.lpSurface, data.data(), data.size());
        }
        else
        {
            for (int y = 0; y < height; y++)
            {
                const uint8_t* srcPtr = data.data() + (y * width);
                uint8_t* dstPtr = static_cast<uint8_t*>(ddsd.lpSurface) + (y * ddsd.lPitch);
                memcpy(dstPtr, srcPtr, width);
            }
        }
        break;
    case 16:
    case 24:
    case 32:
        for (int y = 0; y < height; y++)
        {
            const uint8_t* srcPtr = data.data() + (y * width);
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
                *dstPtr = makeRGB(c.peRed, c.peGreen, c.peBlue);
                srcPtr++;
                dstPtr++;
            }
        }
        break;
    default:
        panic("Unsupported pixel format");
    }

    CHECK(surf->Unlock(nullptr));

    bmp->_surface = surf;
    bmp->_ddsd.dwSize = sizeof(ddsd);
    CHECK(surf->GetSurfaceDesc(&bmp->_ddsd));
    bmp->_dstSurf = _backSurf;
    bmp->_dstWidth = _ddsd.dwWidth;
    bmp->_dstHeight = _ddsd.dwHeight;
    bmp->_bpp = bpp;
    bmp->_pixelFormat = &_pixelFormat;
    bmp->_palette = _paletteEntries;
}

IBitmap* Engine::loadBitmap(const char* filename)
{
    auto bmp = std::make_unique<Bitmap>();
    auto result = bmp.get();
    bmp->_filename = filename;
    reloadBitmap(result);

    _bitmaps.push_back(std::move(bmp));
    return result;
}

ISfx* Engine::loadSfx(const char* filename)
{
    trace("Loading %s", filename);

    int channels, sampleRate;
    int16_t* samples;
    auto sampleCount = stb_vorbis_decode_filename(filename, &channels, &sampleRate, &samples);
    if (sampleCount == -1)
    {
        panic("Failed to decode %s", filename);
    }
    if (sampleRate != 22050)
    {
        panic("Unsupported samplerate for %s: %d", filename, sampleRate);
    }
    else if (channels != 1)
    {
        panic("Unsupported number of channels for %s: %d", filename, channels);
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

    auto sfx = std::make_unique<Sfx>();
    auto result = sfx.get();
    sfx->_sndBuf = sndBuf;
    _sfxs.push_back(std::move(sfx));
    return result;
}

IPalette* Engine::loadPalette(const char* filename)
{
    trace("Loading %s", filename);

    FILE* f = fopen(filename, "rt");
    if (!f)
    {
        panic("Failed to load file %s", filename);
    }

    auto getLine = [f]() -> std::optional<std::string>
    {
        char buffer[512];
        if (!fgets(buffer, sizeof(buffer), f))
        {
            return std::nullopt;
        }

        std::string result(buffer);
        while (!result.empty() && (result.back() == '\r' || result.back() == '\n'))
        {
            result.pop_back();
        }

        return result;
    };

    if (auto header = getLine(); !header || *header != "JASC-PAL")
    {
        panic("Invalid header (magic) for %s", filename);
    }
    else if (auto version = getLine(); !version || *version != "0100")
    {
        panic("Invalid header (version) for %s", filename);
    }
    else if (auto version = getLine(); !version || *version != "256")
    {
        panic("Invalid header (colorcount) for %s", filename);
    }

    auto palette = std::make_unique<Palette>();
    auto result = palette.get();
    palette->_colors.reserve(256);
    for (int i = 0; i < 256; i++)
    {
        auto line = getLine();
        if (!line)
        {
            panic("Failed to read entry %d in %s", i, filename);
        }

        auto tokens = split(*line, " ");
        if (tokens.size() != 3)
        {
            panic("Invalid entry format \"%s\" in %s", line->c_str(), filename);
        }

        auto r = parseInt(tokens[0], 10);
        if (!r)
        {
            panic("Invalid entry format \"%s\" in %s", line->c_str(), filename);
        }

        auto g = parseInt(tokens[1], 10);
        if (!g)
        {
            panic("Invalid entry format \"%s\" in %s", line->c_str(), filename);
        }

        auto b = parseInt(tokens[2], 10);
        if (!b)
        {
            panic("Invalid entry format \"%s\" in %s", line->c_str(), filename);
        }

        Color<uint8_t> color;
        color.r = *r;
        color.g = *g;
        color.b = *b;
        palette->_colors.emplace_back(color);
    }

    _palettes.push_back(std::move(palette));
    return result;
}

void Engine::clearScreen(uint8_t color)
{
    DDBLTFX fx;
    memset(&fx, 0, sizeof(fx));
    fx.dwSize = sizeof(fx);

    int bpp = getBPP(&_ddsd.ddpfPixelFormat);
    switch (bpp)
    {
    case 8:
        fx.dwFillColor = color;
        break;
    case 32:
        fx.dwFillColor = makeRGB(_paletteEntries[color].peRed, _paletteEntries[color].peGreen, _paletteEntries[color].peBlue);
        break;
    default:
        panic("Unsupported pixel format");
    }

    CHECK(_backSurf->Blt(nullptr, nullptr, nullptr, DDBLT_COLORFILL | DDBLT_WAIT, &fx));
}

void Engine::setDebugText(const char* debugText)
{
    _debugText = debugText;
}

KeyState Engine::getKeyState(KeyID key) const
{
    auto it = _keyState.find(key);
    if (it == _keyState.end())
    {
        return KeyState
        {
            false, false
        };
    }

    return it->second;
}

void Engine::setPalette(const IPalette* palette)
{
    if (palette->colorCount() != 256)
    {
        panic("Invalid palette");
    }

    for (int i = 0; i < palette->colorCount(); i++)
    {
        auto color = palette->colorAt(i);
        _paletteEntries[i].peRed = color.r;
        _paletteEntries[i].peGreen = color.g;
        _paletteEntries[i].peBlue = color.b;
        _paletteEntries[i].peFlags = PC_NOCOLLAPSE;
    }

    for (int i = 0; i < 10; i++)
    {
        _paletteEntries[i].peFlags = PC_EXPLICIT;
        _paletteEntries[i].peRed = i;
        _paletteEntries[i].peGreen = _paletteEntries[i].peBlue = 0;

        _paletteEntries[i + 246].peFlags = PC_EXPLICIT;
        _paletteEntries[i + 246].peRed = i + 246;
        _paletteEntries[i + 246].peGreen = _paletteEntries[i + 246].peBlue = 0;
    }

    if (_primarySurf && getBPP(&_ddsd.ddpfPixelFormat) == 0)
    {
        LPDIRECTDRAWPALETTE palette;
        CHECK(_ddraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE, _paletteEntries, &palette, nullptr));
        CHECK(_primarySurf->SetPalette(palette));
        palette->Release();
    }
}

int Engine::run()
{
    CoInitialize(nullptr);

    _game.reset(makeGame());
    GameParams params = _game->getParams();
    _params = params;

    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.hInstance = _hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = nullptr;
    wc.lpfnWndProc = Engine::windowProc;
    wc.lpszClassName = "MainWin";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassEx(&wc))
    {
        return 1;
    }

    RECT rcWindow;
    rcWindow.left = 0;
    rcWindow.top = 0;
    rcWindow.right = params.size.width * _zoom;
    rcWindow.bottom = params.size.height * _zoom;
    AdjustWindowRect(&rcWindow, WINDOWSTYLE, FALSE);

    HWND hwnd = CreateWindowEx(0, "MainWin", params.name, WINDOWSTYLE, 0, 0, rcWindow.right - rcWindow.left, rcWindow.bottom - rcWindow.top, nullptr, nullptr,
                               _hInstance, this);
    if (!hwnd)
    {
        return 1;
    }

    LPDIRECTDRAW ddraw;
    CHECK(DirectDrawCreate(nullptr, &ddraw, nullptr));
    CHECK(ddraw->QueryInterface(IID_IDirectDraw4, (void**)&_ddraw));
    ddraw->Release();
    ddraw = nullptr;

    createSurfaces();

    CHECK(DirectSoundCreate(nullptr, &_dsound, nullptr));
    CHECK(_dsound->SetCooperativeLevel(_hwnd, DSSCL_NORMAL));

    if (!_game->onInit(*this))
    {
        panic("onInit() failed");
    }

    _active = true;

    UpdateWindow(hwnd);
    ShowWindow(hwnd, SW_SHOWDEFAULT);

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
                cleanup();
                return msg.wParam;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (_active)
        {
            auto beginTime = getTime();
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
                _fps = std::round(frames / frameTimer);
                frameTimer = 0;
                frames = 0;
            }

            render();
            prevTime = beginTime;
        }
    }

    cleanup();
    return 0;
}

void Engine::createSurfaces()
{
    freeSurfaces();
    if (_fullscreen)
    {
        CHECK(_ddraw->SetCooperativeLevel(_hwnd, DDSCL_FULLSCREEN | DDSCL_EXCLUSIVE | DDSCL_ALLOWREBOOT | DDSCL_ALLOWMODEX));
        CHECK(_ddraw->SetDisplayMode(_params.size.width, _params.size.height, 8, 0, 0));

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
        windowRect.right = _params.size.width * _zoom;
        windowRect.bottom = _params.size.height * _zoom;
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
        CHECK(_ddraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_INITIALIZE, _paletteEntries, &palette, nullptr));
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
        ddsd.dwWidth = _params.size.width;
        ddsd.dwHeight = _params.size.height;
        ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
        CHECK(_ddraw->CreateSurface(&ddsd, &_backSurf, nullptr));

        LPDIRECTDRAWCLIPPER clipper;
        CHECK(_ddraw->CreateClipper(0, &clipper, nullptr));
        CHECK(clipper->SetHWnd(0, _hwnd));
        CHECK(_primarySurf->SetClipper(clipper));
        clipper->Release();
    }

    _ddsd.dwSize = sizeof(_ddsd);
    CHECK(_backSurf->GetSurfaceDesc(&_ddsd));

    _pixelFormat = makePixelFormat(&_ddsd.ddpfPixelFormat);

    for (auto& i : _bitmaps)
    {
        auto bmp = static_cast<Bitmap*>(i.get());
        reloadBitmap(bmp);
    }
}

void Engine::update(double dT)
{
    if (!_game->onUpdate(*this, dT))
    {
        panic("onUpdate() failed");
    }
}

void Engine::render()
{
    char debugText[255] = "";

    if (_backSurf->IsLost())
    {
        CHECK(_backSurf->Restore());
    }

    if (!_game->onRender(*this, 0.0))
    {
        panic("onRender() failed");
    }

    if (_primarySurf->IsLost())
    {
        _primarySurf->Restore();
    }

    stbsp_snprintf(debugText, sizeof(debugText), "fps=%d", _fps);
    if (!_debugText.empty())
    {
        auto currentLen = strlen(debugText);
        if (currentLen < sizeof(debugText))
        {
            stbsp_snprintf(debugText + currentLen, sizeof(debugText) - currentLen, " %s", _debugText.c_str());
        }
    }

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

        RECT dstRect;
        GetClientRect(_hwnd, &dstRect);
        OffsetRect(&dstRect, origin.x, origin.y);

        RECT srcRect;
        srcRect.left = 0;
        srcRect.top = 0;
        srcRect.right = _ddsd.dwWidth;
        srcRect.bottom = _ddsd.dwHeight;
        REPORT(_primarySurf->Blt(&dstRect, _backSurf, &srcRect, DDBLT_WAIT, nullptr));
    }
}

static void* getWindowLong(HWND hwnd)
{
#ifdef _WIN64
    return reinterpret_cast<void*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
#else
    return reinterpret_cast<void*>(GetWindowLong(hwnd, GWL_USERDATA));
#endif
}

static void setWindowLong(HWND hwnd, void* data)
{
#ifdef _WIN64
    SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)data);
#else
    SetWindowLong(hwnd, GWL_USERDATA, (LONG)data);
#endif
}

LRESULT CALLBACK Engine::windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    auto app = reinterpret_cast<Engine*>(getWindowLong(hwnd));
    if (msg == WM_NCCREATE)
    {
        auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lparam);
        auto app = reinterpret_cast<Engine*>(createStruct->lpCreateParams);
        app->_hwnd = hwnd;
        setWindowLong(hwnd, app);
    }
    else if (msg == WM_NCDESTROY)
    {
        if (app)
        {
            app->_hwnd = nullptr;
        }
        setWindowLong(hwnd, nullptr);
    }
    else
    {
        if (app)
        {
            auto ret = app->onEvent(msg, wparam, lparam);
            if (ret)
            {
                return *ret;
            }
        }
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

std::optional<LRESULT> Engine::onEvent(UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(_hwnd);
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
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_SPACE:
        case VK_ESCAPE:
            onKeyUp(wparam);
            break;
        }
        break;
    case WM_KEYDOWN:
        switch (wparam)
        {
        case VK_LEFT:
        case VK_RIGHT:
        case VK_UP:
        case VK_DOWN:
        case VK_SPACE:
        case VK_ESCAPE:
            onKeyDown(wparam);
            break;
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

void Engine::onKeyUp(int vk)
{
    KeyID keyID;
    switch (vk)
    {
    case VK_LEFT:
        keyID = KeyID::Left;
        break;
    case VK_RIGHT:
        keyID = KeyID::Right;
        break;
    case VK_UP:
        keyID = KeyID::Up;
        break;
    case VK_DOWN:
        keyID = KeyID::Down;
        break;
    case VK_SPACE:
        keyID = KeyID::Space;
        break;
    case VK_ESCAPE:
        keyID = KeyID::Escape;
        break;
    }

    _keyState[keyID].down = false;
    _keyState[keyID].repeat = false;
}

void Engine::onKeyDown(int vk)
{
    KeyID keyID;
    switch (vk)
    {
    case VK_LEFT:
        keyID = KeyID::Left;
        break;
    case VK_RIGHT:
        keyID = KeyID::Right;
        break;
    case VK_UP:
        keyID = KeyID::Up;
        break;
    case VK_DOWN:
        keyID = KeyID::Down;
        break;
    case VK_SPACE:
        keyID = KeyID::Space;
        break;
    case VK_ESCAPE:
        keyID = KeyID::Escape;
        break;
    }

    auto it = _keyState.find(keyID);
    if (it == _keyState.end())
    {
        _keyState[keyID].down = true;
        _keyState[keyID].repeat = false;
    }
    else
    {
        it->second.repeat = it->second.down;
        it->second.down = true;
    }
}

const char* zorro::hresult2str(HRESULT hResult)
{
#define X(X)                                                                                                                                                   \
    case X:                                                                                                                                                    \
        return #X

    switch (hResult)
    {
        X(DDERR_ALREADYINITIALIZED);
        X(DDERR_CANNOTATTACHSURFACE);
        X(DDERR_CANNOTDETACHSURFACE);
        X(DDERR_CURRENTLYNOTAVAIL);
        X(DDERR_EXCEPTION);
        X(DDERR_GENERIC);
        X(DDERR_HEIGHTALIGN);
        X(DDERR_INCOMPATIBLEPRIMARY);
        X(DDERR_INVALIDCAPS);
        X(DDERR_INVALIDCLIPLIST);
        X(DDERR_INVALIDMODE);
        X(DDERR_INVALIDOBJECT);
        X(DDERR_INVALIDPARAMS);
        X(DDERR_INVALIDPIXELFORMAT);
        X(DDERR_INVALIDRECT);
        X(DDERR_LOCKEDSURFACES);
        X(DDERR_NO3D);
        X(DDERR_NOALPHAHW);
        X(DDERR_NOSTEREOHARDWARE);
        X(DDERR_NOSURFACELEFT);
        X(DDERR_NOCLIPLIST);
        X(DDERR_NOCOLORCONVHW);
        X(DDERR_NOCOOPERATIVELEVELSET);
        X(DDERR_NOCOLORKEY);
        X(DDERR_NOCOLORKEYHW);
        X(DDERR_NODIRECTDRAWSUPPORT);
        X(DDERR_NOEXCLUSIVEMODE);
        X(DDERR_NOFLIPHW);
        X(DDERR_NOGDI);
        X(DDERR_NOMIRRORHW);
        X(DDERR_NOTFOUND);
        X(DDERR_NOOVERLAYHW);
        X(DDERR_OVERLAPPINGRECTS);
        X(DDERR_NORASTEROPHW);
        X(DDERR_NOROTATIONHW);
        X(DDERR_NOSTRETCHHW);
        X(DDERR_NOT4BITCOLOR);
        X(DDERR_NOT4BITCOLORINDEX);
        X(DDERR_NOT8BITCOLOR);
        X(DDERR_NOTEXTUREHW);
        X(DDERR_NOVSYNCHW);
        X(DDERR_NOZBUFFERHW);
        X(DDERR_NOZOVERLAYHW);
        X(DDERR_OUTOFCAPS);
        X(DDERR_OUTOFMEMORY);
        X(DDERR_OUTOFVIDEOMEMORY);
        X(DDERR_OVERLAYCANTCLIP);
        X(DDERR_OVERLAYCOLORKEYONLYONEACTIVE);
        X(DDERR_PALETTEBUSY);
        X(DDERR_COLORKEYNOTSET);
        X(DDERR_SURFACEALREADYATTACHED);
        X(DDERR_SURFACEALREADYDEPENDENT);
        X(DDERR_SURFACEBUSY);
        X(DDERR_CANTLOCKSURFACE);
        X(DDERR_SURFACEISOBSCURED);
        X(DDERR_SURFACELOST);
        X(DDERR_SURFACENOTATTACHED);
        X(DDERR_TOOBIGHEIGHT);
        X(DDERR_TOOBIGSIZE);
        X(DDERR_TOOBIGWIDTH);
        X(DDERR_UNSUPPORTED);
        X(DDERR_UNSUPPORTEDFORMAT);
        X(DDERR_UNSUPPORTEDMASK);
        X(DDERR_INVALIDSTREAM);
        X(DDERR_VERTICALBLANKINPROGRESS);
        X(DDERR_WASSTILLDRAWING);
        X(DDERR_DDSCAPSCOMPLEXREQUIRED);
        X(DDERR_XALIGN);
        X(DDERR_INVALIDDIRECTDRAWGUID);
        X(DDERR_DIRECTDRAWALREADYCREATED);
        X(DDERR_NODIRECTDRAWHW);
        X(DDERR_PRIMARYSURFACEALREADYEXISTS);
        X(DDERR_NOEMULATION);
        X(DDERR_REGIONTOOSMALL);
        X(DDERR_CLIPPERISUSINGHWND);
        X(DDERR_NOCLIPPERATTACHED);
        X(DDERR_NOHWND);
        X(DDERR_HWNDSUBCLASSED);
        X(DDERR_HWNDALREADYSET);
        X(DDERR_NOPALETTEATTACHED);
        X(DDERR_NOPALETTEHW);
        X(DDERR_BLTFASTCANTCLIP);
        X(DDERR_NOBLTHW);
        X(DDERR_NODDROPSHW);
        X(DDERR_OVERLAYNOTVISIBLE);
        X(DDERR_NOOVERLAYDEST);
        X(DDERR_INVALIDPOSITION);
        X(DDERR_NOTAOVERLAYSURFACE);
        X(DDERR_EXCLUSIVEMODEALREADYSET);
        X(DDERR_NOTFLIPPABLE);
        X(DDERR_CANTDUPLICATE);
        X(DDERR_NOTLOCKED);
        X(DDERR_CANTCREATEDC);
        X(DDERR_NODC);
        X(DDERR_WRONGMODE);
        X(DDERR_IMPLICITLYCREATED);
        X(DDERR_NOTPALETTIZED);
        X(DDERR_UNSUPPORTEDMODE);
        X(DDERR_NOMIPMAPHW);
        X(DDERR_INVALIDSURFACETYPE);
        X(DDERR_NOOPTIMIZEHW);
        X(DDERR_NOTLOADED);
        X(DDERR_NOFOCUSWINDOW);
        X(DDERR_NOTONMIPMAPSUBLEVEL);
        X(DDERR_DCALREADYCREATED);
        X(DDERR_NONONLOCALVIDMEM);
        X(DDERR_CANTPAGELOCK);
        X(DDERR_CANTPAGEUNLOCK);
        X(DDERR_NOTPAGELOCKED);
        X(DDERR_MOREDATA);
        X(DDERR_EXPIRED);
        X(DDERR_TESTFINISHED);
        X(DDERR_NEWMODE);
        X(DDERR_D3DNOTINITIALIZED);
        X(DDERR_VIDEONOTACTIVE);
        X(DDERR_NOMONITORINFORMATION);
        X(DDERR_NODRIVERSUPPORT);
        X(DDERR_DEVICEDOESNTOWNSURFACE);
        X(DDERR_NOTINITIALIZED);
        X(DD_OK);
        X(DD_FALSE);
    default:
        return "";
    }
#undef X
}

double Engine::getTime() const
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);
    return counter.QuadPart / _hrtFreq;
}

int Engine::getBPP(const DDPIXELFORMAT* pf)
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

uint32_t Engine::makeRGB(uint8_t r, uint8_t g, uint8_t b, const PixelFormat* pf)
{
    if (!pf->valid)
    {
        panic("Invalid _pixelFormat");
    }

    auto rVal = ((r * ((1 << pf->rBits) - 1)) / 255) << pf->rShift;
    auto gVal = ((g * ((1 << pf->gBits) - 1)) / 255) << pf->gShift;
    auto bVal = ((b * ((1 << pf->bBits) - 1)) / 255) << pf->bShift;

    return (rVal & pf->rMask) | (gVal & pf->gMask) | (bVal & pf->bMask);
}

uint32_t Engine::makeRGB(uint8_t r, uint8_t g, uint8_t b)
{
    return makeRGB(r, g, b, &_pixelFormat);
}

PixelFormat Engine::makePixelFormat(const DDPIXELFORMAT* pf)
{
    PixelFormat result;

    // Normalize to the mask size
    result.rMask = pf->dwRBitMask;
    result.gMask = pf->dwGBitMask;
    result.bMask = pf->dwBBitMask;

    // Find bit shifts
    result.rShift = 0;
    while (((result.rMask >> result.rShift) & 1) == 0 && result.rShift < 32)
    {
        result.rShift++;
    }

    result.gShift = 0;
    while (((result.gMask >> result.gShift) & 1) == 0 && result.gShift < 32)
    {
        result.gShift++;
    }

    result.bShift = 0;
    while (((result.bMask >> result.bShift) & 1) == 0 && result.bShift < 32)
    {
        result.bShift++;
    }

    // Scale 0–255 channel values down to mask size
    result.rBits = result.gBits = result.bBits = 0;
    while ((1U << result.rBits) - 1 < (result.rMask >> result.rShift))
    {
        result.rBits++;
    }

    while ((1U << result.gBits) - 1 < (result.gMask >> result.gShift))
    {
        result.gBits++;
    }

    while ((1U << result.bBits) - 1 < (result.bMask >> result.bShift))
    {
        result.bBits++;
    }

    result.valid = true;
    return result;
}

void Engine::freeSurfaces()
{
    _pixelFormat.valid = false;
    memset(&_ddsd, 0, sizeof(_ddsd));

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

    for (auto& i : _bitmaps)
    {
        auto bmp = static_cast<Bitmap*>(i.get());
        if (bmp->_surface)
        {
            bmp->_surface->Release();
            bmp->_surface = nullptr;
        }
        bmp->_dstSurf = nullptr;
    }
}

static char* printCallback(const char* buf, void* userData, int len)
{
    fwrite(buf, len, 1, stdout);
    return (char*)userData;
}

static void vprint(const char* fmt, va_list args)
{
    char printBuf[STB_SPRINTF_MIN];
    stbsp_vsprintfcb(printCallback, printBuf, printBuf, fmt, args);
}

static void print(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vprint(fmt, args);
    va_end(args);
}

static void __vtrace(const char* file, int line, const char* format, va_list args)
{
    char baseFile[MAX_PATH];
    lstrcpyn(baseFile, file, sizeof(baseFile));
    PathStripPathA(baseFile);

    print("[%s:%d] ", baseFile, line);
    vprint(format, args);
    fwrite("\n", 1, 1, stdout);
    fflush(stdout);
}

void zorro::__trace(const char* file, int line, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    __vtrace(file, line, format, args);
    va_end(args);
}

void zorro::__panic(const char* file, int line, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char baseFile[MAX_PATH];
    lstrcpyn(baseFile, file, sizeof(baseFile));
    PathStripPathA(baseFile);

    char buffer[512];
    stbsp_snprintf(buffer, sizeof(buffer), "Fatal error at %s:%d\n\n", baseFile, line);
    stbsp_vsnprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), format, args);
    va_end(args);

    printf("%s\n", buffer);
    fflush(stdout);

    MessageBox(nullptr, buffer, "Panic", MB_OK | MB_ICONERROR);
    ExitProcess(1);
    abort();
    exit(1);
}

void Engine::cleanup()
{
    _game->onCleanup(*this);
    _game.reset();

    freeSurfaces();
    if (_dsound)
    {
        _dsound->Release();
        _dsound = nullptr;
    }

    if (_ddraw)
    {
        _ddraw->Release();
        _ddraw = nullptr;
    }

    for (auto& i : _sfxs)
    {
        auto sfx = static_cast<Sfx*>(i.get());
        if (sfx->_sndBuf)
        {
            sfx->_sndBuf->Release();
            sfx->_sndBuf = nullptr;
        }
    }

    CoUninitialize();
}

void Engine::quit()
{
    PostMessageA(_hwnd, WM_CLOSE, 0, 0);
}

