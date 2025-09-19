#pragma once

#include <ddraw.h>
#include <memory>
#include <optional>
#include <shlwapi.h>
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#define STB_SPRINTF_IMPLEMENTATION
#include "../stb_sprintf.h"

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

static void __trace(const char* file, int line, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    __vtrace(file, line, format, args);
    va_end(args);
}

#define trace(...) __trace(__FILE__, __LINE__, __VA_ARGS__)
// #define trace(...)

[[noreturn]] static void __panic(const char* file, int line, const char* format, ...)
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
}

#define panic(...) __panic(__FILE__, __LINE__, __VA_ARGS__)

inline const char* hresult2str(HRESULT hResult)
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

#define CHECK(fn)                                                                                                                                              \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (auto ret = fn; FAILED(ret))                                                                                                                        \
        {                                                                                                                                                      \
            char buffer[512];                                                                                                                                  \
            stbsp_snprintf(buffer, sizeof(buffer), "%s failed: 0x%lX %s", #fn, ret, hresult2str(ret));                                                         \
            __panic(__FILE__, __LINE__, "%s", buffer);                                                                                                         \
        }                                                                                                                                                      \
    } while (false)

#define RETRY(fn)                                                                                                                                              \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        auto ret = fn;                                                                                                                                         \
        if (FAILED(ret))                                                                                                                                       \
        {                                                                                                                                                      \
            if (ret != DDERR_SURFACEBUSY)                                                                                            \
            {                                                                                                                                                  \
                panic("%s failed: 0x%X %s", #fn, ret, hresult2str(ret));                                                                                       \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            break;                                                                                                                                             \
        }                                                                                                                                                      \
    } while (true)

static double hrTimerFrequency;
inline double getCurrentTime()
{
    LARGE_INTEGER counter;
    if (!QueryPerformanceCounter(&counter))
    {
        panic("QueryPerformanceCounter failed");
    }

    return counter.QuadPart / hrTimerFrequency;
}

class IApp
{
public:
    explicit IApp(HINSTANCE hInstance)
        : _hinstance(hInstance), _hwnd(nullptr)
    {
    }

    virtual ~IApp() = default;
    virtual bool init() = 0;
    virtual int run() = 0;
    virtual void cleanup() = 0;

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

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        auto app = reinterpret_cast<IApp*>(getWindowLong(hwnd));
        if (msg == WM_NCCREATE)
        {
            auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lparam);
            auto app = reinterpret_cast<IApp*>(createStruct->lpCreateParams);
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
                auto ret = app->onEvent(hwnd, msg, wparam, lparam);
                if (ret)
                {
                    return *ret;
                }
            }
        }
        return DefWindowProc(hwnd, msg, wparam, lparam);
    }

protected:
    HINSTANCE _hinstance;
    HWND _hwnd;

    virtual std::optional<LRESULT> onEvent(HWND, UINT, WPARAM, LPARAM) = 0;
};

std::unique_ptr<IApp> makeApp(HINSTANCE hInstance);

INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT)
{
    LARGE_INTEGER freq;
    if (!QueryPerformanceFrequency(&freq))
    {
        panic("QueryPerformanceFrequency failed");
    }
    hrTimerFrequency = freq.QuadPart;

    CoInitialize(nullptr);

    auto app = makeApp(hInstance);
    if (!app->init())
    {
        panic("App::init() failed");
    }

    auto ret = app->run();
    app->cleanup();
    CoUninitialize();
    return ret;
}

