#include <windows.h>
#include <cstdio>
#include <thread>
#include <chrono>

#include "Core/client_instance.h"
#include "logging/logger.h"
#include "shared/version.h"

// Global client instance
std::unique_ptr<HogwartsMP::Core::ClientInstance> gClient = nullptr;
bool gRunning = true;

// Main client thread
void ClientThread() {
    HogwartsMP::Logging::Logger::Info("Client thread started.");

    // Create client instance
    gClient = std::make_unique<HogwartsMP::Core::ClientInstance>();

    HogwartsMP::Core::ClientOptions opts;
    opts.gameName = "Hogwarts Legacy";
    opts.gameVersion = HogwartsMP::Version::rel;
    opts.serverHost = "127.0.0.1";
    opts.serverPort = 27015;

    // Initialize (connects to server)
    if (gClient->Init(opts)) {
        HogwartsMP::Logging::Logger::Info("Client initialized. Entering update loop...");

        // Update loop
        while (gRunning) {
            gClient->Update();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        gClient->Shutdown();
    } else {
        HogwartsMP::Logging::Logger::Error("Failed to initialize client.");
    }

    gClient.reset();
    HogwartsMP::Logging::Logger::Info("Client thread exited.");
    
    // Free console and unload DLL when done
    FreeConsole();
    FreeLibraryAndExitThread(GetModuleHandle(NULL), 0);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved) {
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH: {
        DisableThreadLibraryCalls(hinstDLL);

        // Allocate console for debugging
        AllocConsole();
        SetConsoleTitleW(L"HogwartsMP Client Debug");

        // Initialize logger
        HogwartsMP::Logging::Logger::Initialize("logs", HogwartsMP::Logging::LogLevel::Info);

        // Redirect standard streams to NUL to suppress game/engine logs in our console
        // Our logger uses a direct handle to CONOUT$ so it remains unaffected
        FILE* fDummy;
        freopen_s(&fDummy, "NUL", "w", stdout);
        freopen_s(&fDummy, "NUL", "w", stderr);

        HogwartsMP::Logging::Logger::Info("HogwartsMP DLL Loaded (Clean Network Mode)");

        // Start client thread
        std::thread(ClientThread).detach();
        break;
    }

    case DLL_PROCESS_DETACH: {
        gRunning = false;
        // Give some time for thread to cleanup (rudimentary)
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        break;
    }
    }

    return TRUE;
}
