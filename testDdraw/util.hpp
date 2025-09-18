#pragma once

#include <map>
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

#define CHECK(fn)                                                                                                                                              \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (auto ret = fn; FAILED(ret))                                                                                                                        \
        {                                                                                                                                                      \
            char buffer[512];                                                                                                                                  \
            stbsp_snprintf(buffer, sizeof(buffer), "%s failed: 0x%lX", #fn, ret);                                                                              \
            __panic(__FILE__, __LINE__, "%s", buffer);                                                                                                         \
        }                                                                                                                                                      \
    } while (false)

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

