#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <string>
#include <map>
#include <fstream>

#include "main.h"
#include "wnd_proc.h"
#include "algorithms.h"
#include "resource.h"

#pragma comment(lib, "shlwapi.lib")

static BOOL OnCreate(HWND, LPCREATESTRUCT);                                         // WM_CREATE
static void OnCommand(HWND, int, HWND, UINT);                                       // WM_COMMAND
static void OnPaint(HWND);                                                          // WM_PAINT
static void OnSize(HWND, UINT, int, int);                                           // WM_SIZE
static void OnDestroy(HWND);                                                        // WM_DESTROY

#define IDC_ABOUT 1000
#define IDC_HELP 1001

#define IDC_SEARCH_EDIT 3000
#define IDC_SEARCH 3001
#define IDC_SORT 3002
#define IDC_LIST 4000

static HWND hParentWnd = NULL;

static bool first_uses = true;

enum PROCESS_STATUS { PS_ACTIVE, PS_DROPPED };

typedef struct {
    std::string exe_file;
    std::string execute_time;
    PROCESS_STATUS status;
    std::string cpu;
    std::string virtual_memory;
    DWORD pid;
} PROCESS_INFO;

static std::map<DWORD, PROCESS_INFO> process_list;


// [OnlyProcess_Thread]:
static DWORD WINAPI OnlyProcess_Thread(LPVOID lpObject)
{
    DWORD pid = reinterpret_cast<DWORD>(lpObject);

    while (IsWindow(hParentWnd))
    {
        if (!processIsRunning(pid))
            break;
        Sleep(1000);
    }

    if (!IsWindow(hParentWnd))
        ExitThread(1337);

    HWND hListWnd = GetDlgItem(hParentWnd, IDC_LIST);
    int iItemCount = ListView_GetItemCount(hListWnd);
    for (int i = 0; i < iItemCount; i++)
    {
        char szTemp[1024];
        memset(szTemp, 0, 1024);
        ListView_GetItemText(hListWnd, i, 5, szTemp, sizeof(szTemp));
        if (atoi(szTemp) == pid)
        {
            for (std::map<DWORD, PROCESS_INFO>::iterator p = process_list.begin(); p != process_list.end(); p++)
            {
                if (p->first == pid)
                {
                    p->second.status = PS_DROPPED;
                    char tmp[] = "Остановлен";
                    LPSTR szText = tmp;
                    ListView_SetItemText(hListWnd, i, 2, szText);
                    UpdateWindow(hListWnd);

                    std::ofstream ofs((getThisPath(GetModuleHandle(NULL)) + "\\log.txt").c_str(), std::ios_base::app);
                    ofs.write(p->second.exe_file.c_str(), p->second.exe_file.length());
                    ofs.write(" ::: ", 5);
                    ofs.write(p->second.execute_time.c_str(), p->second.execute_time.length());
                    ofs.write(" ::: ", 5);
                    ofs.write("Остановлен", lstrlen("Остановлен"));
                    ofs.write(" ::: ", 5);
                    ofs.write(p->second.cpu.c_str(), p->second.cpu.length());
                    ofs.write(" ::: ", 5);
                    ofs.write(p->second.virtual_memory.c_str(), p->second.virtual_memory.length());
                    ofs.write(" ::: ", 5);
                    char szTemp[2048];
                    memset(szTemp, 0, 2048);
                    wsprintf(szTemp, "%d", p->second.pid);
                    ofs.write(szTemp, lstrlen(szTemp));
                    ofs.write("\r\n", 2);
                    ofs.close();
                    break;
                }
            }
            break;
        }
    }
    
    ExitThread(1337);
}
// [/OnlyProcess_Thread]


// [firstUses]:
static void firstUses(HWND hWnd)
{
    HWND hListWnd = GetDlgItem(hWnd, IDC_LIST);
    std::ofstream ofs((getThisPath(GetModuleHandle(NULL)) + "\\log.txt").c_str(), std::ios_base::app);

    LV_ITEM lvi;
    RtlZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT;

    PROCESSENTRY32 pe32;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        ofs.close();
        return;
    }
    pe32.dwSize = sizeof(PROCESSENTRY32);

    int nItemIndex = 0;
    char szTemp[2048];
    memset(szTemp, 0, 2048);

    if (!Process32First(hProcessSnap, &pe32))
    {
        if (GetLastError() == ERROR_NO_MORE_FILES)
        {
            CloseHandle(hProcessSnap);
            ofs.close();
            return;
        }
    }

    ListView_DeleteAllItems(hListWnd);

    do {
        lvi.iItem = nItemIndex;
        ListView_InsertItem(hListWnd, &lvi);

        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

        ListView_SetItemText(hListWnd, nItemIndex, 0, pe32.szExeFile);
        ofs.write(pe32.szExeFile, lstrlen(pe32.szExeFile));
        ofs.write(" ::: ", 5);
        process_list[pe32.th32ProcessID].exe_file = pe32.szExeFile;

        std::string execute_time = getExecuteTime(hProcess);
        ListView_SetItemText(hListWnd, nItemIndex, 1, const_cast<LPSTR>(execute_time.c_str()));
        ofs.write(const_cast<LPSTR>(execute_time.c_str()), execute_time.length());
        ofs.write(" ::: ", 5);
        process_list[pe32.th32ProcessID].execute_time = execute_time;

        memset(szTemp, 0, 2048);
        lstrcpy(szTemp, "Активный");
        ListView_SetItemText(hListWnd, nItemIndex, 2, szTemp);
        ofs.write(szTemp, lstrlen(szTemp));
        ofs.write(" ::: ", 5);
        process_list[pe32.th32ProcessID].status = PS_ACTIVE;

        std::string cpu = getCPU(hProcess);
        ListView_SetItemText(hListWnd, nItemIndex, 3, const_cast<LPSTR>(cpu.c_str()));
        ofs.write(const_cast<LPSTR>(cpu.c_str()), cpu.length());
        ofs.write(" ::: ", 5);
        process_list[pe32.th32ProcessID].cpu = cpu;

        std::string VirtualMemory = getRSS(hProcess);
        ListView_SetItemText(hListWnd, nItemIndex, 4, const_cast<LPSTR>(VirtualMemory.c_str()));
        ofs.write(const_cast<LPSTR>(VirtualMemory.c_str()), VirtualMemory.length());
        ofs.write(" ::: ", 5);
        process_list[pe32.th32ProcessID].virtual_memory = VirtualMemory;

        memset(szTemp, 0, 2048);
        wsprintf(szTemp, "%d", pe32.th32ProcessID);
        ListView_SetItemText(hListWnd, nItemIndex, 5, szTemp);
        ofs.write(szTemp, lstrlen(szTemp));
        ofs.write("\r\n", 2);
        process_list[pe32.th32ProcessID].pid = pe32.th32ProcessID;

        DWORD dwThreadId;
        HANDLE hThread = CreateThread(NULL, NULL, OnlyProcess_Thread, (LPVOID)pe32.th32ProcessID, NULL, &dwThreadId);
        if (hThread)
            CloseHandle(hThread);

        CloseHandle(hProcess);
        nItemIndex++;
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    ofs.close();
}
// [/firstUses]


// [refreshTable]:
static void refreshTable(HWND hWnd)
{
    HWND hListWnd = GetDlgItem(hWnd, IDC_LIST);
    std::ofstream ofs((getThisPath(GetModuleHandle(NULL)) + "\\log.txt").c_str(), std::ios_base::app);

    LV_ITEM lvi;
    RtlZeroMemory(&lvi, sizeof(lvi));
    lvi.mask = LVIF_TEXT;

    PROCESSENTRY32 pe32;
    HANDLE hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE)
    {
        ofs.close();
        return;
    }

    int nItemIndex = ListView_GetItemCount(hListWnd);;
    char szTemp[2048];
    memset(szTemp, 0, 2048);

    if (!Process32First(hProcessSnap, &pe32))
    {
        if (GetLastError() == ERROR_NO_MORE_FILES)
        {
            CloseHandle(hProcessSnap);
            ofs.close();
            return;
        }
    }

    do {
        bool isnew = true;

        for (std::map<DWORD, PROCESS_INFO>::const_iterator p = process_list.begin(); p != process_list.end(); p++)
        {
            if (p->first == pe32.th32ProcessID)
                isnew = false;
        }

        if (isnew)
        {
            lvi.iItem = nItemIndex;
            ListView_InsertItem(hListWnd, &lvi);

            HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pe32.th32ProcessID);

            ListView_SetItemText(hListWnd, nItemIndex, 0, pe32.szExeFile);
            ofs.write(pe32.szExeFile, lstrlen(pe32.szExeFile));
            ofs.write(" ::: ", 5);
            process_list[pe32.th32ProcessID].exe_file = pe32.szExeFile;

            std::string execute_time = getExecuteTime(hProcess);
            ListView_SetItemText(hListWnd, nItemIndex, 1, const_cast<LPSTR>(execute_time.c_str()));
            ofs.write(const_cast<LPSTR>(execute_time.c_str()), execute_time.length());
            ofs.write(" ::: ", 5);
            process_list[pe32.th32ProcessID].execute_time = execute_time;

            memset(szTemp, 0, 2048);
            lstrcpy(szTemp, "Активный");
            ListView_SetItemText(hListWnd, nItemIndex, 2, szTemp);
            ofs.write(szTemp, lstrlen(szTemp));
            ofs.write(" ::: ", 5);
            process_list[pe32.th32ProcessID].status = PS_ACTIVE;

            std::string cpu = getCPU(hProcess);
            ListView_SetItemText(hListWnd, nItemIndex, 3, const_cast<LPSTR>(cpu.c_str()));
            ofs.write(const_cast<LPSTR>(cpu.c_str()), cpu.length());
            ofs.write(" ::: ", 5);
            process_list[pe32.th32ProcessID].cpu = cpu;

            std::string VirtualMemory = getRSS(hProcess);
            ListView_SetItemText(hListWnd, nItemIndex, 4, const_cast<LPSTR>(VirtualMemory.c_str()));
            ofs.write(const_cast<LPSTR>(VirtualMemory.c_str()), VirtualMemory.length());
            ofs.write(" ::: ", 5);
            process_list[pe32.th32ProcessID].virtual_memory = VirtualMemory;

            memset(szTemp, 0, 2048);
            wsprintf(szTemp, "%d", pe32.th32ProcessID);
            ListView_SetItemText(hListWnd, nItemIndex, 5, szTemp);
            ofs.write(szTemp, lstrlen(szTemp));
            ofs.write("\r\n", 2);
            process_list[pe32.th32ProcessID].pid = pe32.th32ProcessID;

            DWORD dwThreadId;
            HANDLE hThread = CreateThread(NULL, NULL, OnlyProcess_Thread, (LPVOID)pe32.th32ProcessID, NULL, &dwThreadId);
            if (hThread)
                CloseHandle(hThread);

            CloseHandle(hProcess);
            nItemIndex++;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    /*
    for (std::map<DWORD, PROCESS_INFO>::const_iterator p = process_list.begin(); p != process_list.end(); p++)
    {
        lvi.iItem = nItemIndex;
        ListView_InsertItem(hListWnd, &lvi);
        ListView_SetItemText(hListWnd, nItemIndex, 0, const_cast<LPSTR>(p->second.exe_file.c_str()));

        ListView_SetItemText(hListWnd, nItemIndex, 1, const_cast<LPSTR>(p->second.execute_time.c_str()));

        lstrcpy(szTemp, p->second.status == PS_ACTIVE ? "Активный" : "Остановлен");
        ListView_SetItemText(hListWnd, nItemIndex, 2, szTemp);

        ListView_SetItemText(hListWnd, nItemIndex, 3, const_cast<LPSTR>(p->second.cpu.c_str()));

        ListView_SetItemText(hListWnd, nItemIndex, 4, const_cast<LPSTR>(p->second.virtual_memory.c_str()));

        wsprintf(szTemp, "%d", p->second.pid);
        ListView_SetItemText(hListWnd, nItemIndex, 5, const_cast<LPSTR>(p->second.virtual_memory.c_str()));
    }
    */
    CloseHandle(hProcessSnap);
    ofs.close();
}
// [/refreshTable]


// [ProcessDetect_Thread]:
static DWORD WINAPI ProcessDetect_Thread(LPVOID lpObject)
{
    HWND hWnd = reinterpret_cast<HWND>(lpObject);
    while (IsWindow(hWnd))
    {
        if (first_uses)
        {
            first_uses = false;
            firstUses(hWnd);
        }
        else
        {
            refreshTable(hWnd);
        }
        Sleep(1000);
    }
    ExitThread(1337);
}
// [/ProcessDetect_Thread]


// [SortListView]:
static void SortListView(HWND hListWnd)
{
    char szA[260];
    char szB[260];
    char szTmp[260];
    int iItemCount = ListView_GetItemCount(hListWnd);
    for (int x = 0; x < iItemCount; x++)
    {
        for (int y = 0; y < iItemCount - 1; y++)
        {
            ListView_GetItemText(hListWnd, y, 0, szA, sizeof(szA));
            ListView_GetItemText(hListWnd, y + 1, 0, szB, sizeof(szB));
            if (tolower(szA[0]) > tolower(szB[0]))
            {
                lstrcpy(szTmp, szA);
                lstrcpy(szA, szB);
                lstrcpy(szB, szTmp);
                ListView_SetItemText(hListWnd, y, 0, szA);
                ListView_SetItemText(hListWnd, y + 1, 0, szB);

                for (int i = 1; i < 5; i++)
                {
                    memset(szA, '\0', sizeof(szA));
                    memset(szB, '\0', sizeof(szB));
                    memset(szTmp, '\0', sizeof(szTmp));
                    ListView_GetItemText(hListWnd, y, i, szA, sizeof(szA));
                    ListView_GetItemText(hListWnd, y + 1, i, szB, sizeof(szB));
                    lstrcpy(szTmp, szA);
                    lstrcpy(szA, szB);
                    lstrcpy(szB, szTmp);
                    ListView_SetItemText(hListWnd, y, i, szA);
                    ListView_SetItemText(hListWnd, y + 1, i, szB);
                }
            }
        }
    }
}
// [/SortListView]


// [About_DialogProcedure]:
static BOOL CALLBACK About_DialogProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_INITDIALOG:
    {
        return TRUE;
    }

    case WM_COMMAND:
    {
        switch (LOWORD(wParam))
        {
        case IDOK:
        case IDCANCEL:
            EndDialog(hWnd, 0);
        }
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hDC = BeginPaint(hWnd, &ps);
        RECT rc;
        GetClientRect(hWnd, &rc);
        rc.left += 10;
        rc.right -= 10;
        rc.top += 10;
        rc.bottom -= 10;
        int iOldBkMode = SetBkMode(hDC, TRANSPARENT);
        char szText[] =
            "С помощью программы \"Находка для склеротика\" у Вас есть возможность просмотреть историю запущенных процессов. "
			"После первого запуска программы, на одном уровне с исполняемым файлом, создаётся файл log.txt, в котором хранится история процессов. "
			"Также, при помощи кнопки \"Сортировать\" у Вас есть возможность отсортировать список процессов по названию. "
			"А при помощи кнопки \"Поиск\" можно найти процесс. Для этого необходимо ввести полное название процесса, к примеру svchost.exe. " 
			"При нажатии на кнопку \"Свернуть\" программа продолжит работать в фоновом режиме, чтобы развернуть окно программы - запустите исполняемый файл еще раз.";
        DrawText(hDC, szText, lstrlen(szText), &rc, DT_LEFT | DT_WORDBREAK);
        SetBkMode(hDC, iOldBkMode);
        EndPaint(hWnd, &ps);
    }
    break;

    default:
        return FALSE;
    }
    return TRUE;
}
// [/About_DialogProcedure]


// [WindowProcedure]:
LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        HANDLE_MSG(hWnd, WM_CREATE, OnCreate);
        HANDLE_MSG(hWnd, WM_COMMAND, OnCommand);
        HANDLE_MSG(hWnd, WM_PAINT, OnPaint);
        HANDLE_MSG(hWnd, WM_SIZE, OnSize);
        HANDLE_MSG(hWnd, WM_DESTROY, OnDestroy);
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}
// [/WindowProcedure]


// [OnCreate]: WM_CREATE
static BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpcs)
{
    hParentWnd = hWnd;

    RECT rc;
    GetClientRect(hWnd, &rc);
    int window_width = rc.right - rc.left;
    int window_height = rc.bottom - rc.top;

    HINSTANCE hInstance = GetModuleHandle(NULL);

    HWND hListWnd = CreateWindowEx(0, WC_LISTVIEW, NULL, WS_VISIBLE | WS_CHILD | WS_HSCROLL | WS_VSCROLL | LVS_REPORT, 0, 40, window_width, window_height - 40, hWnd, (HMENU)IDC_LIST, hInstance, NULL);
    
    LV_COLUMN lvc;
    RtlZeroMemory(&lvc, sizeof(lvc));
    lvc.mask = LVCF_FMT | LVCF_SUBITEM | LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;

    lvc.cx = 200;
    lvc.pszText = (LPSTR)  "Название процесса";
    ListView_InsertColumn(hListWnd, 1, &lvc);

    lvc.cx = 200;
    lvc.pszText = (LPSTR) "Дата запуска";
    ListView_InsertColumn(hListWnd, 2, &lvc);

    lvc.cx = 100;
    lvc.pszText = (LPSTR) "Статус";
    ListView_InsertColumn(hListWnd, 3, &lvc);

    lvc.cx = 100;
    lvc.pszText = (LPSTR) "ЦП (%)";
    ListView_InsertColumn(hListWnd, 4, &lvc);

    lvc.cx = 100;
    lvc.pszText = (LPSTR) "ОЗУ (KB)";
    ListView_InsertColumn(hListWnd, 5, &lvc);

    lvc.cx = 100;
    lvc.pszText = (LPSTR) "PID";
    ListView_InsertColumn(hListWnd, 6, &lvc);

    ListView_SetExtendedListViewStyle(hListWnd, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    DWORD dwThreadId;
    HANDLE hThread = CreateThread(NULL, 0, ProcessDetect_Thread, (LPVOID)hWnd, 0, &dwThreadId);
    if (hThread)
        CloseHandle(hThread);

    CreateWindowEx(0, "edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT, 10, 10, window_width - 240, 20, hWnd, (HMENU)IDC_SEARCH_EDIT, NULL, NULL);
    CreateWindowEx(0, "button", "Поиск", WS_VISIBLE | WS_CHILD, window_width - 220, 10, 100, 20, hWnd, (HMENU)IDC_SEARCH, NULL, NULL);
    CreateWindowEx(0, "button", "Сортировать", WS_VISIBLE | WS_CHILD, window_width - 110, 10, 100, 20, hWnd, (HMENU)IDC_SORT, NULL, NULL);

    return TRUE;
}
// [/OnCreate]

// [OnCommand]: WM_COMMAND
static void OnCommand(HWND hWnd, int id, HWND hWndCtl, UINT codeNotify)
{
    switch (id)
    {
    case IDC_ABOUT:
        //DialogBox(GetModuleHandle(NULL), MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)About_DialogProcedure);
		//HtmlHelp(m_hWnd, "Help.chm", HH_DISPLAY_TOPIC, NULL);
		ShellExecute(hWnd, ("open"), ("help.chm"), NULL, NULL, SW_NORMAL);
        break;

	case IDC_HELP:
		ShellExecute(hWnd, ("open"), ("about.chm"), NULL, NULL, SW_NORMAL);
		break;

    case IDC_SORT:
        SortListView(GetDlgItem(hWnd, IDC_LIST));
        break;

    case IDC_SEARCH:
    {
        char szSearchString[260];
        Edit_GetText(GetDlgItem(hWnd, IDC_SEARCH_EDIT), szSearchString, 260);

        LVFINDINFO lvfi;
        RtlZeroMemory(&lvfi, sizeof(LVFINDINFO));
        lvfi.flags = LVFI_STRING;
        lvfi.psz = szSearchString;
        int item = ListView_FindItem(GetDlgItem(hWnd, IDC_LIST), -1, &lvfi);
        if (item == -1)
        {
            MessageBox(hWnd, "Ничего не найдено!", "Уведомление", MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        }
        else
        {
            HWND hListWnd = GetDlgItem(hWnd, IDC_LIST);
            std::string result;
            char buff[1024] = { 0 };

            result += "Название процесса: ";
            memset(buff, 0, 1024);
            ListView_GetItemText(hListWnd, item, 0, buff, 1024);
            result += "\t";
            result += buff;
            result += "\n";

            result += "Дата запуска: ";
            memset(buff, 0, 1024);
            ListView_GetItemText(hListWnd, item, 1, buff, 1024);
            result += "\t\t";
            result += buff;
            result += "\n";

            result += "Статус: ";
            memset(buff, 0, 1024);
            ListView_GetItemText(hListWnd, item, 2, buff, 1024);
            result += "\t\t\t";
            result += buff;
            result += "\n";

            result += "ЦП (%): ";
            memset(buff, 0, 1024);
            ListView_GetItemText(hListWnd, item, 3, buff, 1024);
            result += "\t\t\t";
            result += buff;
            result += "\n";

            result += "ОЗУ (KB): ";
            memset(buff, 0, 1024);
            ListView_GetItemText(hListWnd, item, 4, buff, 1024);
            result += "\t\t\t";
            result += buff;
            result += "\n";

            MessageBox(hWnd, result.c_str(), "Інформация", MB_OK | MB_ICONINFORMATION);
        }
    }
    break;

    default:
        break;
    }
}
// [/OnCommand]


// [OnPaint]: WM_PAINT
static void OnPaint(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC hDC = BeginPaint(hWnd, &ps);

    EndPaint(hWnd, &ps);
}
// [/OnPaint]


// [OnSize]: WM_SIZE
static void OnSize(HWND hWnd, UINT state, int cx, int cy)
{
    if (state == SIZE_MINIMIZED)
        ShowWindow(hWnd, SW_HIDE);

	RECT rt;
	HWND hListWnd = GetDlgItem(hWnd, IDC_LIST);
	HWND hSearchEditWnd = GetDlgItem(hWnd, IDC_SEARCH_EDIT);
	HWND hSearchWnd = GetDlgItem(hWnd, IDC_SEARCH);
	HWND hSortWnd = GetDlgItem(hWnd, IDC_SORT);

	GetClientRect(hWnd, &rt);
	AdjustWindowRectEx(&rt, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_APPWINDOW);
	int window_width = rt.right - rt.left;
	int window_height = rt.bottom - rt.top;
	SetWindowPos(hListWnd, NULL, 0, 40, rt.right - 10, rt.bottom - 50, SWP_NOZORDER);


	DestroyWindow(hSearchEditWnd);
	DestroyWindow(hSearchWnd);
	DestroyWindow(hSortWnd);
	CreateWindowEx(0, "edit", "", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_LEFT, 10, 10, window_width - 260, 20, hWnd, (HMENU)IDC_SEARCH_EDIT, NULL, NULL);
	CreateWindowEx(0, "button", "Поиск", WS_VISIBLE | WS_CHILD, window_width - 240, 10, 100, 20, hWnd, (HMENU)IDC_SEARCH, NULL, NULL);
	CreateWindowEx(0, "button", "Сортировать", WS_VISIBLE | WS_CHILD, window_width - 130, 10, 100, 20, hWnd, (HMENU)IDC_SORT, NULL, NULL);

	/*GetWindowRect(hWnd, &rw);
	MapWindowPoints(HWND_DESKTOP, GetParent(hWnd), (POINT*)&rw, 2);*/

	/*// Get the position of the window relative to the entire screen
	GetWindowRect(hWnd, &rc);

	// Now convert those with regards to the control parent
	ScreenToClient(GetParent(hWnd), (LPPOINT) &((LPPOINT)&rc)[0]);
	ScreenToClient(GetParent(hWnd), (LPPOINT) &((LPPOINT)&rc)[1]);*/
}
// [/OnSize]


// [OnDestroy]: WM_DESTROY
static void OnDestroy(HWND hWnd)
{
    PostQuitMessage(0);
}
// [/OnDestroy]
