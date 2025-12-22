#include "application.h"
#include "../../../shared/logging/logger.h"
#include "../../../shared/version.h"

namespace HogwartsMP::Core {
    Globals gGlobals;
    std::unique_ptr<Application> gApplication = nullptr;

    bool Application::PostInit() {
        Logging::Logger::Info("Application::PostInit called");

        // TODO: Initialize state machine, input, UI when ready
        // TODO: Register ECS modules (Mod, HumanSync, Human)
        // TODO: Setup networking callbacks
        // TODO: Initialize dev features

        Logging::Logger::Info("Application PostInit complete");
        return true;
    }

    bool Application::PreShutdown() {
        Logging::Logger::Info("Application::PreShutdown called");

        // TODO: Cleanup state machine, input, UI
        // TODO: Unregister ECS modules

        return true;
    }

    void Application::PostUpdate() {
        // TODO: Update state machine
        // TODO: Update input system
        // TODO: Process local player updates
    }

    void Application::PostRender() {
        // TODO: Render UI (ImGUI, chat, console)
        // TODO: Render dev features overlay
    }

    void Application::ProcessLockControls(bool lock) {
        if (lock) {
            _lockControlsCounter++;
        } else {
            _lockControlsCounter--;
        }

        _controlsLocked = _lockControlsCounter;
    }

    void Application::LockControls(bool lock) {
        ProcessLockControls(lock);
    }

    void Application::ToggleLockControlsBypass() {
        _lockControlsBypassed = !_lockControlsBypassed;
        Logging::Logger::InfoF("Controls bypass: %s", _lockControlsBypassed ? "enabled" : "disabled");
    }

    uint64_t Application::GetLocalPlayerID() {
        // TODO: Return actual local player ID from ECS
        return 0;
    }

    void Application::InitNetworkingMessages() {
        // TODO: Register network message handlers
        Logging::Logger::Info("Networking messages initialized (placeholder)");
    }

    void Application::InitRPCs() {
        // TODO: Register RPC handlers
        Logging::Logger::Info("RPCs initialized (placeholder)");
    }

} // namespace HogwartsMP::Core
