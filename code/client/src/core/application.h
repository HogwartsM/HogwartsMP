#pragma once

#include <memory>
#include "client_instance.h"

// Forward declarations for UE4 types
class FUObjectArray;

namespace SDK {
    class ULocalPlayer;
    class ABiped_Player;
    class UWorld;
}

namespace HogwartsMP::Core {

    class Application : public ClientInstance {
      private:
        friend class DevFeatures;

        flecs::entity _localPlayer;
        float _tickInterval = 0.01667f;

        int _controlsLocked = 0;
        int _lockControlsCounter = 0;
        bool _lockControlsBypassed = false;

      private:
        void ProcessLockControls(bool lock);
        void InitNetworkingMessages();
        void InitRPCs();

      public:
        // ClientInstance overrides
        bool PostInit() override;
        bool PreShutdown() override;
        void PostUpdate() override;
        void PostRender() override;

        // Accessors
        float GetTickInterval() const {
            return _tickInterval;
        }

        // Controls management
        void LockControls(bool lock);
        bool AreControlsLocked() const {
            return _controlsLocked > 0;
        }
        void ToggleLockControlsBypass();
        bool AreControlsLockedBypassed() const {
            return _lockControlsBypassed;
        }

        uint64_t GetLocalPlayerID();
    };

    // Global pointers for UE4 integration
    struct Globals {
        Application* application = nullptr;
        HWND window = nullptr;
        ID3D12Device* device = nullptr;
        FUObjectArray* objectArray = nullptr;
        SDK::ULocalPlayer* localPlayer = nullptr;
        SDK::ABiped_Player* localBipedPlayer = nullptr;
        SDK::UWorld** world = nullptr;
    };

    extern Globals gGlobals;
    extern std::unique_ptr<Application> gApplication;
}
