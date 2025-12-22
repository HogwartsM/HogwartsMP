#include <windows.h>
#include <MinHook.h>
#include <crtdbg.h>
#include <cstdio>

#include "Core/application.h"

// Custom CRT Report Hook to log to console and trigger JIT
int __cdecl HogwartsMPReportHook(int reportType, char* message, int* returnValue) {
    // Log to console (stderr)
    if (message) {
        fprintf(stderr, "%s", message);
    }

    // If it's an error or assertion, trigger JIT (Breakpoint)
    if (reportType == _CRT_ASSERT || reportType == _CRT_ERROR) {
        // Trigger a breakpoint. 
        // If a debugger is attached, it breaks here.
        // If not, it triggers Windows Error Reporting (JIT) if enabled.
        __debugbreak();
        return TRUE; // Return TRUE to suppress the default message box
    }

    // For warnings, just print and continue (suppress message box)
    return TRUE; 
}
#include "logging/logger.h"
#include "shared/version.h"

// Hook system base address
namespace hook {
    static uintptr_t g_base = 0;

    void set_base(uintptr_t base = 0) {
        if (base == 0) {
            g_base = reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
        } else {
            g_base = base;
        }
    }

    uintptr_t get_base() {
        return g_base;
    }
}

// Init functions system defined in utils/hooking/hook_function.h
#include <utils/hooking/hook_function.h>

typedef char *(__fastcall *GetNarrowWinMainCommandLine_t)();
GetNarrowWinMainCommandLine_t GetNarrowWinMainCommandLine_original = nullptr;

char *GetNarrowWinMainCommandLine() {
    // Run all init functions (hooks setup)
    InitFunction::RunAll();
    MH_EnableHook(MH_ALL_HOOKS);

    // Create our core module application
    HogwartsMP::Core::gApplication.reset(new HogwartsMP::Core::Application);
    if (HogwartsMP::Core::gApplication && !HogwartsMP::Core::gApplication->IsInitialized()) {
        HogwartsMP::Core::ClientOptions opts;
        opts.discordAppId = 1076503389606789130;
        opts.useRenderer = true;
        opts.useImGUI = true;
        opts.gameName = "Hogwarts Legacy";
        opts.gameVersion = HogwartsMP::Version::rel;
        opts.renderer.backend = HogwartsMP::Core::RendererBackend::D3D12;
        opts.renderer.platform = HogwartsMP::Core::PlatformBackend::Win32;

        HogwartsMP::Core::gApplication->Init(opts);

        HogwartsMP::Logging::Logger::InfoF("HogwartsMP Client initialized - %s v%s",
                                          opts.gameName.c_str(),
                                          opts.gameVersion.c_str());
    }

    return GetNarrowWinMainCommandLine_original();
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
        // Show MessageBox to confirm DLL loaded
        // MessageBoxW(NULL, L"HogwartsMP DLL chargée !\nLa console va s'ouvrir.", L"HogwartsMP", MB_OK | MB_ICONINFORMATION);

        // Try to attach to parent console (Launcher)
        if (!AttachConsole(ATTACH_PARENT_PROCESS)) {
            // If failed (e.g. launched directly), allocate a new console
            AllocConsole();
        }

        // Redirect standard IO to the console
    // FILE* fDummy;
    // freopen_s(&fDummy, "CONIN$", "r", stdin);
    // freopen_s(&fDummy, "CONOUT$", "w", stdout);
    // freopen_s(&fDummy, "CONOUT$", "w", stderr);

    // Install Custom CRT Report Hook
        _CrtSetReportHook(HogwartsMPReportHook);
        
        // Also redirect standard assert just in case
        _set_error_mode(_OUT_TO_STDERR);

        SetConsoleTitleW(L"HogwartsMP Client");

        // Initialize logging
        HogwartsMP::Logging::Logger::Initialize("logs", HogwartsMP::Logging::LogLevel::Info);
        HogwartsMP::Logging::Logger::Info("HogwartsMP DLL loaded");

        // Initialize MinHook
        MH_Initialize();

        // Set base address for hooking
        auto base = GetModuleHandle(nullptr);
        hook::set_base(reinterpret_cast<uintptr_t>(base));

        // Hook the WinMain command line function to delay initialization
        // This ensures we initialize after Denuvo unpacking
        auto handle = LoadLibrary(TEXT("api-ms-win-crt-runtime-l1-1-0.dll"));
        if (handle) {
            auto procAddr = GetProcAddress((HMODULE)handle, "_get_narrow_winmain_command_line");
            if (procAddr) {
                MH_CreateHook(reinterpret_cast<LPVOID>(procAddr),
                             reinterpret_cast<LPVOID>(&GetNarrowWinMainCommandLine),
                             reinterpret_cast<LPVOID*>(&GetNarrowWinMainCommandLine_original));
                MH_EnableHook(reinterpret_cast<LPVOID>(procAddr));
                HogwartsMP::Logging::Logger::Info("WinMain hook installed");
            } else {
                HogwartsMP::Logging::Logger::Error("Failed to find _get_narrow_winmain_command_line");
            }
        } else {
            HogwartsMP::Logging::Logger::Error("Failed to load api-ms-win-crt-runtime-l1-1-0.dll");
        }

        // Initialize client directly (fallback if WinMain hook doesn't trigger)
        // This creates a thread to initialize after a delay
        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            Sleep(3000); // Wait 3 seconds for game to initialize

            if (!HogwartsMP::Core::gApplication || !HogwartsMP::Core::gApplication->IsInitialized()) {
                HogwartsMP::Logging::Logger::Info("WinMain hook didn't trigger, initializing client directly...");

                // Run all init functions (hooks setup)
                InitFunction::RunAll();
                MH_EnableHook(MH_ALL_HOOKS);

                // Create our core module application
                HogwartsMP::Core::gApplication.reset(new HogwartsMP::Core::Application);
                if (HogwartsMP::Core::gApplication) {
                    HogwartsMP::Core::ClientOptions opts;
                    opts.discordAppId = 1076503389606789130;
                    opts.useRenderer = true;
                    opts.useImGUI = true;
                    opts.gameName = "Hogwarts Legacy";
                    opts.gameVersion = HogwartsMP::Version::rel;
                    opts.renderer.backend = HogwartsMP::Core::RendererBackend::D3D12;
                    opts.renderer.platform = HogwartsMP::Core::PlatformBackend::Win32;

                    HogwartsMP::Core::gApplication->Init(opts);

                    HogwartsMP::Logging::Logger::InfoF("HogwartsMP Client initialized (fallback) - %s v%s",
                                                      opts.gameName.c_str(),
                                                      opts.gameVersion.c_str());
                }
            }

            return 0;
        }, nullptr, 0, nullptr);

    } break;

    case DLL_PROCESS_DETACH: {
        if (HogwartsMP::Core::gApplication) {
            HogwartsMP::Core::gApplication->Shutdown();
            HogwartsMP::Core::gApplication.reset();
        }

        MH_Uninitialize();
        HogwartsMP::Logging::Logger::Info("HogwartsMP DLL unloaded");
    } break;
    }

    return TRUE;
}
