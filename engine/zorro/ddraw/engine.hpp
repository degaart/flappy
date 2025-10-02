#pragma once

#include <zorro/IEngine.hpp>
#include <zorro/IGame.hpp>
#include <zorro/stb_sprintf.h>
#include <zorro/util.hpp>
#include <memory>
#include <ddraw.h>
#include <mmeapi.h>
#include <dsound.h>
#include <windows.h>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace zorro
{
    struct PixelFormat
    {
        bool valid;
        int rBits, gBits, bBits;
        int rShift, gShift, bShift;
        unsigned rMask, gMask, bMask;
    };

    struct Bitmap : IBitmap
    {
        int width() const override;
        int height() const override;
        void blt(int dstX, int dstY,
                 int srcX, int srcY, int srcWidth, int srcHeight,
                 int colorKey) override;

        std::string _filename;
        LPDIRECTDRAWSURFACE4 _surface;
        DDSURFACEDESC2 _ddsd;
        LPDIRECTDRAWSURFACE4 _dstSurf;
        int _dstWidth;
        int _dstHeight;
        int _bpp;
        const PixelFormat* _pixelFormat;
        PALETTEENTRY* _palette;
    };

    struct Sfx : ISfx
    {
        void setFreq(int freq) override;
        void play() override;
        void stop() override;

        LPDIRECTSOUNDBUFFER _sndBuf;
    };

    struct Palette : IPalette
    {
        int colorCount() const override;
        Color<uint8_t> colorAt(int index) const override;

        std::vector<Color<uint8_t>> _colors;
    };

    struct Engine : public IEngine
    {
    public:
        explicit Engine(HINSTANCE hInstance);
        Engine(Engine&&) = delete;
        Engine(const Engine&) = delete;
        Engine& operator=(Engine&&) = delete;
        Engine& operator=(const Engine&) = delete;

        /* Lifetime is managed by engine */
        IBitmap* loadBitmap(const char* filename) override;

        /* Lifetime is managed by engine */
        ISfx* loadSfx(const char* filename) override;

        /* Lifetime is managed by engine */
        IPalette* loadPalette(const char* filename) override;

        void clearScreen(uint8_t color) override;
        void setDebugText(const char* debugText) override;
        KeyState getKeyState(KeyID key) const override;
        void setPalette(const IPalette* palette) override;
        double getTime() const override;
        void quit() override;
        int run();


        static uint32_t makeRGB(uint8_t r, uint8_t g, uint8_t b, const PixelFormat* pf);

    private:
        HINSTANCE _hInstance;
        double _hrtFreq;
        std::unique_ptr<IGame> _game;
        GameParams _params;
        int _zoom;
        bool _active;
        int _fps;
        bool _fullscreen;
        HWND _hwnd;
        std::map<KeyID,KeyState> _keyState;
        PALETTEENTRY _paletteEntries[256];
        std::vector<std::unique_ptr<IBitmap>> _bitmaps;
        std::vector<std::unique_ptr<ISfx>> _sfxs;
        std::vector<std::unique_ptr<IPalette>> _palettes;
        std::string _debugText;

        LPDIRECTDRAW4 _ddraw;
        LPDIRECTDRAWSURFACE4 _primarySurf;
        LPDIRECTDRAWSURFACE4 _backSurf;
        DDSURFACEDESC2 _ddsd; /* for _backSurf */
        PixelFormat _pixelFormat;

        LPDIRECTSOUND _dsound;

        static constexpr auto WINDOWSTYLE = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

        static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
        std::optional<LRESULT> onEvent(UINT msg, WPARAM wparam, LPARAM lparam);
        void createSurfaces();
        void freeSurfaces();
        void update(double dT);
        void render();
        static int getBPP(const DDPIXELFORMAT* pf);
        uint32_t makeRGB(uint8_t r, uint8_t g, uint8_t b);
        void onKeyUp(int vk);
        void onKeyDown(int vk);
        static PixelFormat makePixelFormat(const DDPIXELFORMAT* pf);
        void cleanup();
        void reloadBitmap(Bitmap* bitmap);
    };

    const char* hresult2str(HRESULT hResult);
}

#define CHECK(fn) \
    if (auto ret = fn; FAILED(ret)) { \
        char buffer[512]; \
        stbsp_snprintf(buffer, sizeof(buffer), "%s failed: 0x%lX %s", #fn, ret, zorro::hresult2str(ret));                                                         \
        zorro::__panic(__FILE__, __LINE__, "%s", buffer); \
    }

#define REPORT(fn) \
    if (auto ret = fn; FAILED(ret)) { \
        char buffer[512]; \
        stbsp_snprintf(buffer, sizeof(buffer), "%s failed: 0x%lX %s", #fn, ret, zorro::hresult2str(ret));                                                         \
        zorro::__trace(__FILE__, __LINE__, "%s", buffer);                                                                                                         \
    }

