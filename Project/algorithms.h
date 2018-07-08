#pragma once

#include <windows.h>
#include <psapi.h>
#include <shlwapi.h>
#include <string>
#include <sstream>

BOOL setPrivilege(HANDLE hToken, LPCTSTR lpszPrivilege, BOOL bEnablePrivilege);
BOOL processIsRunning(DWORD pid);
std::string getThisPath(HINSTANCE hInstance);
std::string getExecuteTime(HANDLE hProcess);
std::string getCPU(HANDLE hProcess);
std::string getRSS(HANDLE hProcess);
