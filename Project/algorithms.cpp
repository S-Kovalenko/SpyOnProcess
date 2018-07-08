#include "algorithms.h"

#pragma comment(lib, "winmm.lib")

// [setPrivilege]:
BOOL setPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege)
{
    LUID luid;
    if (!LookupPrivilegeValue(NULL, lpszPrivilege, &luid))
        return FALSE;

    TOKEN_PRIVILEGES tp;
    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    if (bEnablePrivilege)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    if (!AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), (PTOKEN_PRIVILEGES)NULL, (PDWORD)NULL))
        return FALSE;

    if (GetLastError() == ERROR_NOT_ALL_ASSIGNED)
        return FALSE;
    return TRUE;
}
// [/setPrivilege]


// [processIsRunning]:
BOOL processIsRunning(DWORD pid)
{
    HANDLE process = OpenProcess(SYNCHRONIZE, FALSE, pid);
    DWORD ret = WaitForSingleObject(process, 0);
    CloseHandle(process);
    return ret == WAIT_TIMEOUT;
}
// [/processIsRunning]


// [getThisPath]:
std::string getThisPath(HINSTANCE hInstance)
{
    char szTemp[2048] = { 0 };
    if (GetModuleFileName(hInstance, szTemp, 2048) > 0)
    {
        PathRemoveFileSpec(szTemp);
        return szTemp;
    }
    return "";
}
// [/getThisPath]


// [getExecuteTime]:
std::string getExecuteTime(HANDLE hProcess)
{
    if (!hProcess)
        return "Неизвестно";

    FILETIME file_time[4];
    if (GetProcessTimes(hProcess, &file_time[0], &file_time[1], &file_time[2], &file_time[3]))
    {
        SYSTEMTIME system_time_utc[4];
        SYSTEMTIME system_time_local[4];

        for (int i = 0; i < 4; i++)
        {
            FileTimeToSystemTime(&file_time[i], &system_time_utc[i]);
            SystemTimeToTzSpecificLocalTime(NULL, &system_time_utc[i], &system_time_local[i]);
        }

        char szTemp[256];
        memset(szTemp, 0, 256);
        wsprintf(szTemp, "%s%d.%s%d.%d %s%d:%s%d:%s%d",
            system_time_local[0].wDay <= 9 ? "0" : "", system_time_local[0].wDay,
            system_time_local[0].wMonth <= 9 ? "0" : "", system_time_local[0].wMonth,
            system_time_local[0].wYear,
            system_time_local[0].wHour <= 9 ? "0" : "", system_time_local[0].wHour,
            system_time_local[0].wMinute <= 9 ? "0" : "", system_time_local[0].wMinute,
            system_time_local[0].wSecond <= 9 ? "0" : "", system_time_local[0].wSecond);

        return std::string(szTemp);
    }

    return "Неизвестно";
}
// [/getExecuteTime]


// [getCPU]:
std::string getCPU(HANDLE hProcess)
{
    FILETIME ftCreate, ftExit, ftUser, ftKernel, ftNewUser, ftNewKernel;

    DWORD dwOldTime = timeGetTime();
    GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftUser, &ftKernel);

    DWORD dwNewTime = timeGetTime();
    GetProcessTimes(hProcess, &ftCreate, &ftExit, &ftNewUser, &ftNewKernel);

    LONG lKernel = ftNewKernel.dwLowDateTime - ftKernel.dwLowDateTime;
    LONG lUser = ftNewUser.dwLowDateTime - ftUser.dwLowDateTime;
    LONG lProcTime = lKernel + lUser;
    DWORD dwTime = dwNewTime - dwOldTime;

    if (dwTime == 0)
    {
        Sleep(100);
        dwNewTime = timeGetTime();
        dwTime = dwNewTime - dwOldTime;
    }

    int iProcUsage = 0;
    if (dwTime != 0)
        iProcUsage = ((lProcTime * 100) / dwTime);

    if (iProcUsage > 100)
        iProcUsage = 0;

    std::stringstream ss;
    ss << iProcUsage;
    return ss.str();
}
// [/getCPU]


// [getRSS]: get resident set size
std::string getRSS(HANDLE hProcess)
{
    PROCESS_MEMORY_COUNTERS pmc;
    if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc)))
    {
        SIZE_T VirtualMemory = pmc.WorkingSetSize >> 10; // pmc.WorkingSetSize / 1024;
        std::stringstream ss;
        ss << VirtualMemory;
        return ss.str();
    }
    return "";
}
// [/getRSS]
