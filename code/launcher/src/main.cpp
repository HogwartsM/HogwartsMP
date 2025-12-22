#include <windows.h>
#include <tlhelp32.h>
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include "manual_map.h"
#include "Logger.h"

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
        LOG_ERROR_W(L"Failed to allocate memory in target process: " << GetLastError());
        return false;
    }

    // Write DLL path to target process
    if (!WriteProcessMemory(hProcess, pRemoteLibPath, dllPath.c_str(),
                           (dllPath.length() + 1) * sizeof(wchar_t), NULL)) {
        LOG_ERROR_W(L"Failed to write DLL path: " << GetLastError());
        VirtualFreeEx(hProcess, pRemoteLibPath, 0, MEM_RELEASE);
        return false;
    }

    // Get LoadLibraryW address (same in all processes)
    HMODULE hKernel32 = GetModuleHandleW(L"kernel32.dll");
    FARPROC pLoadLibraryW = GetProcAddress(hKernel32, "LoadLibraryW");
    if (!pLoadLibraryW) {
        LOG_ERROR_W(L"Failed to get LoadLibraryW address");
        VirtualFreeEx(hProcess, pRemoteLibPath, 0, MEM_RELEASE);
        return false;
    }

    // Create remote thread to load DLL
    HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0,
                                       (LPTHREAD_START_ROUTINE)pLoadLibraryW,
                                       pRemoteLibPath, 0, NULL);
    if (!hThread) {
        LOG_ERROR_W(L"Failed to create remote thread: " << GetLastError());
        VirtualFreeEx(hProcess, pRemoteLibPath, 0, MEM_RELEASE);
        return false;
    }

    // Wait for injection to complete
    DWORD waitResult = WaitForSingleObject(hThread, 10000); // 10 second timeout

    if (waitResult == WAIT_TIMEOUT) {
        LOG_ERROR_W(L"Injection timeout - thread may be blocked by anti-cheat");
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
        LOG_ERROR_W(L"LoadLibrary failed in target process");
        LOG_ERROR_W(L"Possible causes:");
        LOG_ERROR_W(L"  1. DLL dependencies missing (check MinGW DLLs)");
        LOG_ERROR_W(L"  2. Anti-cheat (Denuvo) blocking DLL load");
        LOG_ERROR_W(L"  3. DLL architecture mismatch (must be x64)");
        LOG_ERROR_W(L"  4. DLL corrupted or path incorrect");

        // Try to get more info from target process
        LOG_ERROR_W(L"\nDLL path sent to target: " << dllPath);
        return false;
    }

    LOG_INFO_W(L"DLL loaded successfully at address: 0x" << std::hex << exitCode << std::dec);
    return true;
}

int main() {
    Logger::Init();
    LOG_INFO_W(L"=== HogwartsMP Launcher v2.0.0 ===");
    LOG_INFO_W(L"Denuvo bypass injection method");

    // Get paths
    std::wstring gamePath = GetGamePath();
    std::wstring dllPath = GetDllPath();

    if (gamePath.empty()) {
        LOG_ERROR_W(L"ERROR: Could not find Hogwarts Legacy installation!");
        LOG_ERROR_W(L"Please specify game path manually.");
        std::wcout << L"Enter full path to HogwartsLegacy.exe: ";
        std::getline(std::wcin, gamePath);

        if (!std::filesystem::exists(gamePath)) {
            LOG_ERROR_W(L"ERROR: Game not found at specified path!");
            Logger::Shutdown();
            system("pause");
            return 1;
        }
    }

    if (!std::filesystem::exists(dllPath)) {
        LOG_ERROR_W(L"ERROR: libHogwartsMPClient.dll not found!");
        LOG_ERROR_W(L"Expected location: " << dllPath);
        Logger::Shutdown();
        system("pause");
        return 1;
    }

    LOG_INFO_W(L"Game path: " << gamePath);
    LOG_INFO_W(L"DLL path: " << dllPath);

    // Get working directory (game folder)
    std::filesystem::path gameDir = std::filesystem::path(gamePath).parent_path();

    // Create process in suspended state
    LOG_INFO_W(L"Launching game in suspended mode...");

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
        LOG_ERROR_W(L"Failed to launch game: " << GetLastError());
        Logger::Shutdown();
        system("pause");
        return 1;
    }

    LOG_INFO_W(L"Game process created (PID: " << pi.dwProcessId << L")");

    // Wait a bit for process to initialize
    Sleep(1000);

    // Inject DLL using Manual Map (bypasses Denuvo detection)
    LOG_INFO_W(L"Injecting HogwartsMP DLL using Manual Map...");

    if (!ManualMapInject(pi.hProcess, dllPath)) {
        LOG_ERROR_W(L"DLL injection failed!");
        TerminateProcess(pi.hProcess, 1);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        Logger::Shutdown();
        system("pause");
        return 1;
    }

    LOG_INFO_W(L"DLL injected successfully!");
    LOG_INFO_W(L"Resuming game execution...");

    // Resume game execution
    ResumeThread(pi.hThread);

    LOG_INFO_W(L"Game launched successfully!");
    LOG_INFO_W(L"HogwartsMP is now active.");
    std::wcout << L"You can close this window." << std::endl;

    // Close handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    Logger::Shutdown();
    return 0;
}
