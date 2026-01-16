#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <memory>
#include <vector>
#include "screenRender.hpp"
#include "readObj.hpp"
#pragma comment (lib,"Gdiplus.lib")

constexpr UINT_PTR IDT_TIMER1 = 1;

struct WindowData {
    Camera camera;
    std::vector<worldTriangle> triangleArray;
    std::vector<Gdiplus::ARGB> imageArray;
    std::vector<float> depthBuffer;
    size_t width;
    size_t height;
    WindowData(Camera _camera,
               std::vector<worldTriangle> _triangleArray)
               : camera(_camera),
               triangleArray(_triangleArray) {}
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

INT WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, PSTR, INT iCmdShow) {
    using namespace Gdiplus;
    HWND                hWnd;
    MSG                 msg;
    WNDCLASS            wndClass;
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR           gdiplusToken;
   
    // Initialize GDI+.
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    wndClass.style          = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc    = WndProc;
    wndClass.cbClsExtra     = 0;
    wndClass.cbWndExtra     = 0;
    wndClass.hInstance      = hInstance;
    wndClass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground  = nullptr;
    wndClass.lpszMenuName   = NULL;
    wndClass.lpszClassName  = TEXT("GettingStarted");

    RegisterClass(&wndClass);
    RAWINPUTDEVICE rawInput[2];

    rawInput[0].usUsagePage = 0x01; //Generic
    rawInput[0].usUsage = 0x02; //Mouse
    rawInput[0].dwFlags = 0;
    rawInput[0].hwndTarget = 0;

    rawInput[1].usUsagePage = 0x01; //Generic
    rawInput[1].usUsage = 0x06; //Keyboard
    rawInput[1].dwFlags = 0;
    rawInput[1].hwndTarget = 0;

    RegisterRawInputDevices(rawInput, ARRAYSIZE(rawInput), sizeof(rawInput[0]));

    Camera camera(0,0,5,
                  0,180,0,
                  80,0.5,100);
    std::vector<worldTriangle> triangleArray;

    WindowData* windowData = new WindowData (camera, triangleArray);

    objToTriangles("model", windowData->triangleArray);

    hWnd = CreateWindow(
        TEXT("GettingStarted"),   // window class name
        TEXT("Getting Started"),  // window caption
        WS_OVERLAPPEDWINDOW,      // window style
        CW_USEDEFAULT,            // initial x position
        CW_USEDEFAULT,            // initial y position
        CW_USEDEFAULT,            // initial x size
        CW_USEDEFAULT,            // initial y size
        NULL,                     // parent window handle
        NULL,                     // window menu handle
        hInstance,                // program instance handle
        windowData);              // window data

        SetTimer(hWnd, IDT_TIMER1, 17, (TIMERPROC) NULL);

    ShowWindow(hWnd, iCmdShow);
    UpdateWindow(hWnd);

    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
   
    GdiplusShutdown(gdiplusToken);
    return msg.wParam;
}  // WinMain

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, 
    WPARAM wParam, LPARAM lParam)
    {
    HDC          hdc;
    PAINTSTRUCT  ps;

    switch(message) {
    case WM_CREATE:
        {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*> (lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (cs->lpCreateParams));
        WindowData* windowData = reinterpret_cast<WindowData*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
        auto& imageArray = windowData->imageArray;
        auto& depthBuffer = windowData->depthBuffer;
        RECT rect;
        GetClientRect(hWnd, &rect);
        windowData->width = rect.right;
        windowData->height = rect.bottom;
        imageArray.resize(rect.right * rect.bottom);
        depthBuffer.resize(rect.right * rect.bottom);
        }
        return 0;
    case WM_PAINT:
        {
        hdc = BeginPaint(hWnd, &ps);

        WindowData* windowData = reinterpret_cast<WindowData*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
        auto& camera = windowData->camera;
        auto& triangles = windowData->triangleArray;
        auto& imageArray = windowData->imageArray;
        auto& depthBuffer = windowData->depthBuffer;

        RECT rect;
        GetClientRect(hWnd, &rect);
        if(windowData->width != rect.right || windowData->height != rect.bottom) {
            imageArray.resize(rect.right * rect.bottom);
            depthBuffer.resize(rect.right * rect.bottom);
            windowData->width = rect.right;
            windowData->height = rect.bottom;
        }

        renderImage(camera, triangles, rect.right, rect.bottom, imageArray, depthBuffer);

        OnPaint(hdc, rect.right, rect.bottom, imageArray, camera);

        EndPaint(hWnd, &ps);
        }
        return 0;
    case WM_INPUT:
        {
        WindowData* windowData = reinterpret_cast<WindowData*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
        Camera &camera = windowData->camera;

        UINT dwSize = sizeof(RAWINPUT);
        static BYTE lpb[sizeof(RAWINPUT)];
        GetRawInputData(reinterpret_cast<HRAWINPUT> (lParam), RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));
        RAWINPUT* raw = reinterpret_cast<RAWINPUT*> (lpb);

        if(raw->header.dwType == RIM_TYPEMOUSE) {
            camera.updateViewAngle(raw->data.mouse.lLastX, raw->data.mouse.lLastY);
        }

        if(raw->header.dwType == RIM_TYPEKEYBOARD) {
            camera.updateCameraPos(hWnd, raw->data.keyboard.Message, raw->data.keyboard.VKey);
        }
        }
        return 0;
    case WM_TIMER:
        {
        switch(reinterpret_cast<UINT_PTR> (wParam)) {
        case IDT_TIMER1:
            InvalidateRect(hWnd, NULL, FALSE);
        }
        }
        return 0;
    case WM_DESTROY:
        {
        WindowData* windowData = reinterpret_cast<WindowData*> (GetWindowLongPtr(hWnd, GWLP_USERDATA));
        SetWindowLongPtr(hWnd, GWLP_USERDATA, 0);
        if(windowData != nullptr) delete windowData;

        KillTimer(hWnd, IDT_TIMER1);

        PostQuitMessage(0);
        }
        return 0;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
} // WndProc