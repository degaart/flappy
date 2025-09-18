#include <stdio.h>
#include <windows.h>

LRESULT CALLBACK windowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_KEYUP:
        if (wparam == VK_F2)
        {
            MessageBox(nullptr, "Totosy", "Error", MB_OK);
            ExitProcess(1);
        }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wparam, lparam);
}

INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, INT)
{
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize = sizeof(wc);
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hIcon = LoadIcon(nullptr, IDI_WINLOGO);
    wc.hIconSm = wc.hIcon;
    wc.hbrBackground = nullptr;
    wc.lpfnWndProc = windowProc;
    wc.lpszClassName = "MainWin";
    wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassEx(&wc))
    {
        return 1;
    }

    auto windowStyle = WS_OVERLAPPEDWINDOW;

    RECT winRect;
    winRect.left = winRect.top = 0;
    winRect.right = 640;
    winRect.bottom = 480;
    AdjustWindowRect(&winRect, windowStyle, FALSE);

    HWND hwnd = CreateWindowEx(0, "MainWin", "Zinzolu", windowStyle, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);
    if (!hwnd)
    {
        return 1;
    }
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}

