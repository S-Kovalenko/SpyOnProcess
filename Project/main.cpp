#include <windows.h>
#include <windowsx.h>
#include <shlobj.h>

#include "main.h"
#include "wnd_proc.h"
#include "algorithms.h"
#include "resource.h"

#pragma comment(lib, "comctl32.lib")


// [WinMain]:
int WINAPI WinMain(
    _In_        HINSTANCE hInstance,
    _In_opt_    HINSTANCE hPrevInstance,
    _In_        LPSTR lpszArgument,
    _In_        int nFunsterStil)
{
    InitCommonControls();

    const char szWindowTitle[] = "Spy on the processes";
    const char szWindowClassName[] = "j2k031cl_spy_on_the_processes_class";

    RECT rc;
    SetRect(&rc, 0, 0, 820, 500);
	DWORD dwWindowStyle = WS_OVERLAPPEDWINDOW;

    AdjustWindowRectEx(&rc, dwWindowStyle, FALSE, WS_EX_APPWINDOW);
    const int iWindowWidth = rc.right - rc.left;
    const int iWindowHeight = rc.bottom - rc.top;


    if (!IsUserAnAdmin())
    {
        MessageBox(NULL, "Запустите программу от имени администратора", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST);
        return -1;
    }

    HWND hWnd = FindWindow(szWindowClassName, NULL);
    if (hWnd != NULL)
    {
        ShowWindow(hWnd, SW_SHOW);
        SetForegroundWindow(hWnd);
        return 0;
    }

    HANDLE hToken = NULL;
    OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken);
    if (!hToken || !setPrivilege(hToken, "SeDebugPrivilege", TRUE))
    {
        MessageBox(NULL, "Не удалось установить привилегию SeDebugPrivilege", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST);
        return -1;
    }


    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WindowProcedure;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
    wcex.lpszClassName = szWindowClassName;
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex))
    {
        MessageBox(NULL, "Не удалось зарегистрировать класс для окна!", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST);
        setPrivilege(hToken, "SeDebugPrivilege", FALSE);
        return -1;
    }

    hWnd = CreateWindowEx(
        0,
        szWindowClassName,
        szWindowTitle,
        dwWindowStyle,
        (GetSystemMetrics(SM_CXSCREEN) >> 1) - (iWindowWidth >> 1),
        (GetSystemMetrics(SM_CYSCREEN) >> 1) - (iWindowHeight >> 1),
        iWindowWidth,
        iWindowHeight,
        HWND_DESKTOP,
        NULL,
        hInstance,
        NULL);
    if (!hWnd)
    {
        MessageBox(NULL, "Не удалось создать окно!", "Error!", MB_OK | MB_ICONERROR | MB_TOPMOST);
        UnregisterClass(szWindowClassName, hInstance);
        setPrivilege(hToken, "SeDebugPrivilege", FALSE);
        return -1;
    }

    ShowWindow(hWnd, SW_SHOWNORMAL);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        DispatchMessage(&msg);
        TranslateMessage(&msg);
    }

    UnregisterClass(szWindowClassName, hInstance);
    setPrivilege(hToken, "SeDebugPrivilege", FALSE);
    return (int)msg.wParam;
}
// [/WinMain]
