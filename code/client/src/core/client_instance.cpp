#include "client_instance.h"
#include "../../../shared/logging/logger.h"
#include "../Services/network_client.h"
#include <vector>
#include <windows.h>
#include <psapi.h>
#include <chrono>

// SDK structures simplified for reading
namespace SDK {
    struct FVector {
        float X;
        float Y;
        float Z;
    };
}

namespace HogwartsMP::Core {

ClientInstance::ClientInstance() = default;
ClientInstance::~ClientInstance() = default;

bool ClientInstance::Init(ClientOptions opts) {
    if (_initialized) {
        Logging::Logger::Warning("ClientInstance already initialized.");
        return false;
    }

    _options = opts;

    Logging::Logger::InfoF("Initializing Client for %s v%s", _options.gameName.c_str(), _options.gameVersion.c_str());

    // Initialize network client
    _networkClient = std::make_unique<Networking::NetworkClient>();
    
    _networkClient->SetOnConnected([]() {
        Logging::Logger::Info("CONNECTED TO SERVER!");
    });

    _networkClient->SetOnDisconnected([]() {
        Logging::Logger::Info("DISCONNECTED FROM SERVER.");
    });

    _networkClient->SetOnPacketReceived([this](Networking::PacketType type, const uint8_t* data, size_t size) {
        // Logging::Logger::InfoF("Packet received: Type %d, Size %zu", static_cast<int>(type), size);
        
        if (type == Networking::PacketType::RequestPlayerCoordinates) {
            LogPlayerLocation();
        }
    });

    if (!_networkClient->Connect(_options.serverHost, _options.serverPort)) {
        Logging::Logger::Error("Failed to initiate connection to server.");
        return false;
    }

    _initialized = true;
    Logging::Logger::Info("Client initialized successfully (Network Only).");
    return true;
}

void ClientInstance::Update() {
    if (!_initialized) return;

    if (_networkClient) {
        _networkClient->Update();
    }
}

void ClientInstance::LogPlayerLocation() {
    static uintptr_t gWorldAddress = 0;
    
    if (gWorldAddress == 0) {
        MODULEINFO modInfo;
        GetModuleInformation(GetCurrentProcess(), GetModuleHandle(nullptr), &modInfo, sizeof(MODULEINFO));
        
        auto scan = [](uintptr_t start, size_t length, const char* pattern, const char* mask) -> uintptr_t {
            for (size_t i = 0; i < length; i++) {
                bool found = true;
                for (size_t j = 0; mask[j] != '\0'; j++) {
                    if (mask[j] != '?' && pattern[j] != *reinterpret_cast<char*>(start + i + j)) {
                        found = false;
                        break;
                    }
                }
                if (found) return start + i;
            }
            return 0;
        };

        const char* pattern = "\x48\x8B\x1D\x00\x00\x00\x00\x48\x85\xDB\x74\x3B\x41\xB0\x01";
        const char* mask = "xxx????xxxxxxxx";
        
        uintptr_t result = scan(reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll), modInfo.SizeOfImage, pattern, mask);
        
        if (result != 0) {
            int32_t offset = *reinterpret_cast<int32_t*>(result + 3);
            gWorldAddress = result + 7 + offset;
            Logging::Logger::InfoF("Found GWorld at: %p (Base + %llX)", (void*)gWorldAddress, gWorldAddress - reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll));
        } else {
             const char* p2 = "\x48\x8B\x05\x00\x00\x00\x00\x48\x3B\xC3\x48\x0F\x44\xC3\x48\x89\x05";
             const char* m2 = "xxx????xxxxxxxxx";
              
             result = scan(reinterpret_cast<uintptr_t>(modInfo.lpBaseOfDll), modInfo.SizeOfImage, p2, m2);
             if (result != 0) {
                int32_t offset = *reinterpret_cast<int32_t*>(result + 3);
                gWorldAddress = result + 7 + offset;
                Logging::Logger::InfoF("Found GWorld (Pattern 2) at: %p", (void*)gWorldAddress);
             } else {
                 Logging::Logger::Warning("GWorld pattern scan failed!");
                 return;
             }
        }
    }

    uintptr_t uWorld = *reinterpret_cast<uintptr_t*>(gWorldAddress);
    if (!uWorld) {
        Logging::Logger::DebugF("UWorld is null at %p", (void*)gWorldAddress);
        return;
    }

    uintptr_t gameInstance = *reinterpret_cast<uintptr_t*>(uWorld + 0x1B8);
    if (!gameInstance) {
        Logging::Logger::DebugF("GameInstance is null (UWorld + 0x1B8)");
        return;
    }
    
    uintptr_t localPlayersArray = *reinterpret_cast<uintptr_t*>(gameInstance + 0x38);
    int32_t localPlayersCount = *reinterpret_cast<int32_t*>(gameInstance + 0x38 + 0x8);
    
    if (!localPlayersArray || localPlayersCount <= 0) {
        Logging::Logger::DebugF("LocalPlayers array empty or null (Count: %d)", localPlayersCount);
        return;
    }
    
    uintptr_t localPlayer = *reinterpret_cast<uintptr_t*>(localPlayersArray);
    if (!localPlayer) {
        Logging::Logger::Debug("LocalPlayer[0] is null");
        return;
    }
    
    uintptr_t playerController = *reinterpret_cast<uintptr_t*>(localPlayer + 0x30);
    if (!playerController) {
        Logging::Logger::Debug("PlayerController is null");
        return;
    }
    
    uintptr_t pawn = *reinterpret_cast<uintptr_t*>(playerController + 0x2C8);
    if (!pawn) {
        Logging::Logger::Debug("AcknowledgedPawn is null (Player not spawned?)");
        return;
    }
    
    uintptr_t rootComponent = *reinterpret_cast<uintptr_t*>(pawn + 0x158);
    if (!rootComponent) {
        Logging::Logger::Debug("RootComponent is null");
        return;
    }
    
    SDK::FVector* location = reinterpret_cast<SDK::FVector*>(rootComponent + 0x1F0);
    
    // Check if values are sane (avoid printing garbage if offset is wrong)
    if (abs(location->X) < 10000000.0f && abs(location->Y) < 10000000.0f && abs(location->Z) < 10000000.0f) {
        Logging::Logger::InfoF("Player Location: X=%.2f, Y=%.2f, Z=%.2f", location->X, location->Y, location->Z);
    } else {
        Logging::Logger::WarningF("Player Location seems invalid (Offset wrong?): X=%.2f, Y=%.2f, Z=%.2f", location->X, location->Y, location->Z);
    }
}

void ClientInstance::Shutdown() {
    if (!_initialized) return;

    Logging::Logger::Info("Shutting down ClientInstance...");

    if (_networkClient) {
        _networkClient->Disconnect();
        _networkClient.reset();
    }

    _initialized = false;
}

} // namespace HogwartsMP::Core
