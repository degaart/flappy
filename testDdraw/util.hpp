#pragma once

#include <memory>
#include <optional>
#include <shlwapi.h>
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

[[noreturn]] inline void __panic(const char* file, int line, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    char baseFile[MAX_PATH];
    lstrcpyn(baseFile, file, sizeof(baseFile));
    PathStripPathA(baseFile);

    char buffer[512];
    snprintf(buffer, sizeof(buffer), "Fatal error at %s:%d\n\n", baseFile, line);
    vsnprintf(buffer + strlen(buffer), sizeof(buffer) - strlen(buffer), format, args);
    va_end(args);

    MessageBoxA(nullptr, buffer, "Panic", MB_OK | MB_ICONERROR);
    ExitProcess(1);
}

#define panic(...) __panic(__FILE__, __LINE__, __VA_ARGS__)
#define CHECK(fn) \
    do { \
        if (auto ret = fn; FAILED(ret)) { \
            char buffer[512]; \
            snprintf(buffer, sizeof(buffer), "%s failed: 0x%lX", #fn, ret); \
            __panic(__FILE__, __LINE__, "%s", buffer); \
        } \
    } while(false)

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

    static LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
    {
        if (msg == WM_NCCREATE)
        {
            auto createStruct = reinterpret_cast<LPCREATESTRUCT>(lparam);
#ifdef _WIN64
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)createStruct->lpCreateParams);
#else
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)createStruct->lpCreateParams);
#endif

            auto app = reinterpret_cast<IApp*>(createStruct->lpCreateParams);
            app->_hwnd = hwnd;
        }
        else if(msg == WM_NCDESTROY)
        {
#ifdef _WIN64
            SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
#else
            SetWindowLong(hwnd, GWL_USERDATA, (LONG)0);
#endif
        }
        else
        {
#ifdef _WIN64
            auto app = reinterpret_cast<IApp*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
#else
            auto app = reinterpret_cast<IApp*>(GetWindowLong(hwnd, GWL_USERDATA));
#endif
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

