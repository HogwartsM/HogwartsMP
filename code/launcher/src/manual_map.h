#pragma once
#include <windows.h>
#include <string>

// Manual Map injection - bypasses LoadLibrary detection by Denuvo
// Maps DLL directly into target process memory without using Windows loader APIs

struct ManualMapData {
    LPVOID imageBase;
    HMODULE(WINAPI* fnLoadLibraryA)(LPCSTR);
    FARPROC(WINAPI* fnGetProcAddress)(HMODULE, LPCSTR);
};

// Function executed in target process to finalize DLL mapping
DWORD WINAPI LoaderStub(LPVOID pData);

// Manual map a DLL into target process
bool ManualMapInject(HANDLE hProcess, const std::wstring& dllPath);
