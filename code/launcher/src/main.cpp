#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "manual_map.h"

// Advanced launcher with Manual Map injection
// Bypasses Denuvo by mapping DLL directly without LoadLibrary
// Uses CREATE_SUSPENDED to inject before anti-cheat initializes

std::wstring GetGamePath() {
    // TODO: Add Steam library detection
    // For now, user must have game in default location or set via config

    // Try common Steam install locations
    std::vector<std::wstring> possiblePaths = {
        L"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Hogwarts Legacy\\Phoenix\\Binaries\\Win64\\HogwartsLegacy.exe",
        L"D:\\SteamLibrary\\steamapps\\common\\Hogwarts Legacy\\Phoenix\\Binaries\\Win64\\HogwartsLegacy.exe",
        L"E:\\SteamLibrary\\steamapps\\common\\Hogwarts Legacy\\Phoenix\\Binaries\\Win64\\HogwartsLegacy.exe"
    };

    for (const auto& path : possiblePaths) {
        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    return L"";
}

std::wstring GetDllPath() {
    wchar_t exePath[MAX_PATH];
    GetModuleFileNameW(NULL, exePath, MAX_PATH);

    std::filesystem::path launcherPath(exePath);
    std::filesystem::path dllPath = launcherPath.parent_path() / L"libHogwartsMPClient.dll";

    return dllPath.wstring();
}

bool InjectDLL(HANDLE hProcess, const std::wstring& dllPath) {
    // Allocate memory in target process for DLL path
    void* pRemoteLibPath = VirtualAllocEx(hProcess, NULL, (dllPath.length() + 1) * sizeof(wchar_t),
                                          MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!pRemoteLibPath) {
        std::wcerr << L"Failed to allocate memory in target process: " << GetLastError() << std::endl;
        return false;
    }

    // Write DLL path to target process
    if (!WriteProcessMemory(hProcess, pRemoteLibPath, dllPath.c_str(),
                           (dllPath.length() + 1) * sizeof(wchar_t), NULL)) {
        std::wcerr << L"Failed to write DLL path: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteLibPath, 0, MEM_RELEASE);
        return false;
    }

    // Get LoadLibraryW address (same in all processes)
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibraryW) {
        std::wcerr << L"Failed to get LoadLibraryW address" << std::endl;
        VirtualFreeEx(hProcess, pRemoteLibPath, 0, MEM_RELEASE);
        return false;
    }

    // Create remote thread to load DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
                                       (LPTHREAD_START_ROUTINE)pLoadLibraryW,
                                       pRemoteLibPath, 0, NULL);
    if (!hThread) {
        std::wcerr << L"Failed to create remote thread: " << GetLastError() << std::endl;
        VirtualFreeEx(hProcess, pRemoteLibPath, 0, MEM_RELEASE);
        return false;
    }

    // Wait for injection to complete
    DWORD waitResult = WaitForSingleObject(hThread, 10000); // 10 second timeout

    if (waitResult == WAIT_TIMEOUT) {
        std::wcerr << L"Injection timeout - thread may be blocked by anti-cheat" << std::endl;
        CloseHandle(hThread);
        VirtualFreeEx(hProcess, pRemoteLibPath, 0, MEM_RELEASE);
        return false;
    }

    // Get LoadLibrary return value (module handle)
    DWORD exitCode = 0;
    GetExitCodeThread(hThread, &exitCode);

    CloseHandle(hThread);
    VirtualFreeEx(hProcess, pRemoteLibPath, 0, MEM_RELEASE);

    if (exitCode == 0) {
        std::wcerr << L"LoadLibrary failed in target process" << std::endl;
        std::wcerr << L"Possible causes:" << std::endl;
        std::wcerr << L"  1. DLL dependencies missing (check MinGW DLLs)" << std::endl;
        std::wcerr << L"  2. Anti-cheat (Denuvo) blocking DLL load" << std::endl;
        std::wcerr << L"  3. DLL architecture mismatch (must be x64)" << std::endl;
        std::wcerr << L"  4. DLL corrupted or path incorrect" << std::endl;

        // Try to get more info from target process
        std::wcerr << L"\nDLL path sent to target: " << dllPath << std::endl;
        return false;
    }

    std::wcout << L"DLL loaded successfully at address: 0x" << std::hex << exitCode << std::dec << std::endl;
    return true;
}

int main() {
    std::wcout << L"=== HogwartsMP Launcher v2.0.0 ===" << std::endl;
    std::wcout << L"Denuvo bypass injection method" << std::endl << std::endl;

    // Get paths
    std::wstring gamePath = GetGamePath();
    std::wstring dllPath = GetDllPath();

    if (gamePath.empty()) {
        std::wcerr << L"ERROR: Could not find Hogwarts Legacy installation!" << std::endl;
        std::wcerr << L"Please specify game path manually." << std::endl;
        std::wcout << L"Enter full path to HogwartsLegacy.exe: ";
        std::getline(std::wcin, gamePath);

        if (!std::filesystem::exists(gamePath)) {
            std::wcerr << L"ERROR: Game not found at specified path!" << std::endl;
            system("pause");
            return 1;
        }
    }

    if (!std::filesystem::exists(dllPath)) {
        std::wcerr << L"ERROR: libHogwartsMPClient.dll not found!" << std::endl;
        std::wcerr << L"Expected location: " << dllPath << std::endl;
        system("pause");
        return 1;
    }

    std::wcout << L"Game path: " << gamePath << std::endl;
    std::wcout << L"DLL path: " << dllPath << std::endl << std::endl;

    // Get working directory (game folder)
    std::filesystem::path gameDir = std::filesystem::path(gamePath).parent_path();

    // Create process in suspended state
    std::wcout << L"Launching game in suspended mode..." << std::endl;

    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi = { 0 };

    if (!CreateProcessW(
        gamePath.c_str(),
        NULL,
        NULL,
        NULL,
        FALSE,
        CREATE_SUSPENDED,  // Launch suspended to inject before Denuvo
        NULL,
        gameDir.c_str(),
        &si,
        &pi)) {
        std::wcerr << L"Failed to launch game: " << GetLastError() << std::endl;
        system("pause");
        return 1;
    }

    std::wcout << L"Game process created (PID: " << pi.dwProcessId << L")" << std::endl;

    // Wait a bit for process to initialize
    Sleep(1000);

    // Inject DLL using Manual Map (bypasses Denuvo detection)
    std::wcout << L"Injecting HogwartsMP DLL using Manual Map..." << std::endl;

    if (!ManualMapInject(pi.hProcess, dllPath)) {
        std::wcerr << L"DLL injection failed!" << std::endl;
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        system("pause");
        return 1;
    }

    std::wcout << L"DLL injected successfully!" << std::endl;
    std::wcout << L"Resuming game execution..." << std::endl;

    // Resume game execution
    ResumeThread(pi.hThread);

    std::wcout << L"Game launched successfully!" << std::endl;
    std::wcout << L"HogwartsMP is now active." << std::endl << std::endl;
    std::wcout << L"You can close this window." << std::endl;

    // Close handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}
