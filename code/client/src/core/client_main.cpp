/**
 * @file client_main.cpp
 * @brief Client principal HogwartsMP v2.0 - Utilise entièrement le framework
 */

#include <windows.h>
#include <thread>
#include <chrono>

// Framework v2.0
#include <framework/utilities/include/DebugSystem.h>
#include <framework/components/networking/include/NetLibrary.h>
#include <framework/components/networking/include/NetBuffer.h>
#include <framework/components/entities/include/EntityManager.h>
#include <framework/components/entities/include/BaseEntity.h>
#include <framework/components/game/include/SpellSystem.h>
#include <framework/natives/include/ClientNatives.h>
#include <framework/events/include/EventManager.h>
#include <framework/memory/include/GameOffsets.h>

using namespace HogwartsMP::Framework;

// ============================================================================
// CLIENT INSTANCE
// ============================================================================

class HogwartsMPClient {
public:
    HogwartsMPClient() = default;
    ~HogwartsMPClient() = default;

    bool Initialize() {
        // 1. Debug System
        auto& debug = Utilities::DebugSystem::Get();
        debug.SetEnabled(true);
        debug.SetDebugLevel(Utilities::DebugLevel::Detailed);
        debug.EnableAllCategories(true);

        DEBUG_LOG(Utilities::DebugCategory::None,
                  Utilities::DebugLevel::Basic,
                  "=== HogwartsMP Client v2.0 ===");

        // 2. Game Offsets
        auto& offsets = Memory::GameOffsets::Get();
        if (!offsets.Initialize()) {
            DEBUG_LOG(Utilities::DebugCategory::Memory,
                      Utilities::DebugLevel::Basic,
                      "Warning: Some game offsets not found");
        }

        // 3. Spell System
        auto& spellSys = Game::SpellSystem::Get();
        spellSys.Initialize();

        DEBUG_LOG(Utilities::DebugCategory::Game,
                  Utilities::DebugLevel::Basic,
                  "Spell system initialized");

        // 4. Network Client
        _netClient = std::make_unique<Networking::NetLibraryClient>();
        _netClient->SetServerAddress("127.0.0.1", 7777);
        _netClient->SetTimeout(5000);

        DEBUG_LOG(Utilities::DebugCategory::Network,
                  Utilities::DebugLevel::Basic,
                  "Network client configured");

        // 5. Register network handlers
        SetupNetworkHandlers();

        // 6. Register debug commands
        RegisterDebugCommands();

        // 7. Register events
        RegisterEvents();

        DEBUG_LOG(Utilities::DebugCategory::None,
                  Utilities::DebugLevel::Basic,
                  "Client initialized successfully");

        return true;
    }

    bool Connect() {
        if (!_netClient) return false;

        DEBUG_LOG(Utilities::DebugCategory::Network,
                  Utilities::DebugLevel::Basic,
                  "Connecting to server...");

        if (_netClient->Connect()) {
            DEBUG_LOG(Utilities::DebugCategory::Network,
                      Utilities::DebugLevel::Basic,
                      "Connected to server!");

            // Créer le joueur local
            CreateLocalPlayer();

            return true;
        }

        DEBUG_LOG(Utilities::DebugCategory::Network,
                  Utilities::DebugLevel::Basic,
                  "Failed to connect to server");

        return false;
    }

    void Update(float deltaTime) {
        DEBUG_PROFILE_SCOPE("ClientUpdate");

        if (!_running) return;

        // Update réseau
        if (_netClient && _netClient->IsConnected()) {
            DEBUG_PROFILE_SCOPE("NetworkUpdate");
            _netClient->Update();
        }

        // Update entités
        {
            DEBUG_PROFILE_SCOPE("EntityUpdate");
            Entities::EntityManager::Get().Update(deltaTime);
        }

        // Update sorts
        {
            DEBUG_PROFILE_SCOPE("SpellUpdate");
            Game::SpellSystem::Get().Update(deltaTime);
        }

        // Synchroniser le joueur local
        if (_localPlayerId != 0) {
            SyncLocalPlayer();
        }
    }

    void Shutdown() {
        DEBUG_LOG(Utilities::DebugCategory::None,
                  Utilities::DebugLevel::Basic,
                  "Shutting down client...");

        _running = false;

        if (_netClient && _netClient->IsConnected()) {
            // Envoyer la déconnexion
            Networking::NetBuffer buffer;
            buffer.WriteUInt32(_localPlayerId);
            _netClient->SendPacket(0, Networking::PacketType::Disconnect, buffer);

            _netClient->Disconnect();
        }

        Entities::EntityManager::Get().Clear();

        // Performance report
        DEBUG_LOG(Utilities::DebugCategory::None,
                  Utilities::DebugLevel::Basic,
                  "=== Performance Report ===");
        Utilities::DebugSystem::Get().PrintPerformanceReport();

        DEBUG_LOG(Utilities::DebugCategory::None,
                  Utilities::DebugLevel::Basic,
                  "Client shutdown complete");
    }

    bool IsRunning() const { return _running; }
    void Stop() { _running = false; }

private:
    void SetupNetworkHandlers() {
        // Entity Create
        _netClient->RegisterPacketHandler(Networking::PacketType::EntityCreate,
            [this](uint32_t peerId, Networking::NetBuffer& buffer) {
                uint32_t entityId = buffer.ReadUInt32();
                uint8_t typeValue = buffer.ReadByte();
                std::string model = buffer.ReadString();
                float x, y, z;
                buffer.ReadVector3(x, y, z);

                auto type = static_cast<Entities::EntityType>(typeValue);
                auto& entityMgr = Entities::EntityManager::Get();

                uint32_t localEntityId = entityMgr.CreateEntity(type, model);
                entityMgr.SetEntityPosition(localEntityId, FVector(x, y, z));

                DEBUG_LOGF(Utilities::DebugCategory::Entities,
                          Utilities::DebugLevel::Detailed,
                          "Remote entity %d created (local: %d)", entityId, localEntityId);
            }
        );

        // Entity Update
        _netClient->RegisterPacketHandler(Networking::PacketType::EntityUpdate,
            [](uint32_t peerId, Networking::NetBuffer& buffer) {
                uint32_t entityId = buffer.ReadUInt32();
                float x, y, z;
                buffer.ReadVector3(x, y, z);

                auto& entityMgr = Entities::EntityManager::Get();
                if (entityMgr.DoesEntityExist(entityId)) {
                    entityMgr.SetEntityPosition(entityId, FVector(x, y, z));
                }
            }
        );

        // Chat Message
        _netClient->RegisterPacketHandler(Networking::PacketType::ChatMessage,
            [](uint32_t peerId, Networking::NetBuffer& buffer) {
                std::string sender = buffer.ReadString();
                std::string message = buffer.ReadString();

                DEBUG_LOGF(Utilities::DebugCategory::Network,
                          Utilities::DebugLevel::Basic,
                          "[%s]: %s", sender.c_str(), message.c_str());
            }
        );

        DEBUG_LOG(Utilities::DebugCategory::Network,
                  Utilities::DebugLevel::Detailed,
                  "Network handlers registered");
    }

    void RegisterDebugCommands() {
        auto& debug = Utilities::DebugSystem::Get();

        // Teleport command
        debug.RegisterCommand("tp", [this](const std::vector<std::string>& args) {
            if (args.size() < 4) {
                DEBUG_LOG(Utilities::DebugCategory::None,
                         Utilities::DebugLevel::Basic,
                         "Usage: tp <x> <y> <z>");
                return;
            }

            float x = std::stof(args[1]);
            float y = std::stof(args[2]);
            float z = std::stof(args[3]);

            Natives::TeleportPlayer(x, y, z);

            DEBUG_LOGF(Utilities::DebugCategory::Game,
                      Utilities::DebugLevel::Basic,
                      "Teleported to (%.1f, %.1f, %.1f)", x, y, z);
        });

        // Position command
        debug.RegisterCommand("pos", [](const std::vector<std::string>& args) {
            FVector pos = Natives::GetPlayerCoords();
            DEBUG_LOGF(Utilities::DebugCategory::Game,
                      Utilities::DebugLevel::Basic,
                      "Position: (%.1f, %.1f, %.1f)", pos.X, pos.Y, pos.Z);
        });

        // Cast spell command
        debug.RegisterCommand("cast", [this](const std::vector<std::string>& args) {
            if (args.size() < 2) {
                DEBUG_LOG(Utilities::DebugCategory::None,
                         Utilities::DebugLevel::Basic,
                         "Usage: cast <spell_name>");

                auto& spellSys = Game::SpellSystem::Get();
                auto spells = spellSys.GetAllSpells();
                DEBUG_LOG(Utilities::DebugCategory::None,
                         Utilities::DebugLevel::Basic,
                         "Available spells:");
                for (const auto& spell : spells) {
                    DEBUG_LOGF(Utilities::DebugCategory::None,
                              Utilities::DebugLevel::Basic,
                              "  - %s", spell.c_str());
                }
                return;
            }

            auto& spellSys = Game::SpellSystem::Get();
            if (spellSys.CanCastSpell(_localPlayerId, args[1])) {
                uint32_t spellId = spellSys.CastSpell(_localPlayerId, args[1], 0);
                DEBUG_LOGF(Utilities::DebugCategory::Game,
                          Utilities::DebugLevel::Basic,
                          "Cast %s (ID: %d)", args[1].c_str(), spellId);
            } else {
                DEBUG_LOG(Utilities::DebugCategory::Game,
                         Utilities::DebugLevel::Basic,
                         "Cannot cast spell (cooldown or invalid)");
            }
        });

        DEBUG_LOG(Utilities::DebugCategory::None,
                  Utilities::DebugLevel::Detailed,
                  "Debug commands registered");
    }

    void RegisterEvents() {
        auto& eventMgr = Events::EventManager::Get();

        eventMgr.RegisterEventHandler("playerSpawned", [](const Events::EventArgs& args) {
            DEBUG_LOG(Utilities::DebugCategory::Events,
                      Utilities::DebugLevel::Basic,
                      "Event: Player spawned");
        });

        DEBUG_LOG(Utilities::DebugCategory::Events,
                  Utilities::DebugLevel::Detailed,
                  "Events registered");
    }

    void CreateLocalPlayer() {
        auto& entityMgr = Entities::EntityManager::Get();

        // Créer l'entité joueur
        _localPlayerId = entityMgr.CreateEntity(
            Entities::EntityType::Player,
            "PlayerCharacter"
        );

        // Configurer
        auto* player = static_cast<Entities::PlayerEntity*>(
            entityMgr.GetEntity(_localPlayerId)
        );

        if (player) {
            player->SetPlayerName("LocalPlayer");
            player->SetNetworked(true);

            // Position depuis le jeu
            FVector pos = Natives::GetPlayerCoords();
            player->SetPosition(pos);

            DEBUG_LOGF(Utilities::DebugCategory::Entities,
                      Utilities::DebugLevel::Basic,
                      "Local player created (ID: %d)", _localPlayerId);

            // Notifier le serveur
            Networking::NetBuffer buffer;
            buffer.WriteByte(static_cast<uint8_t>(Entities::EntityType::Player));
            buffer.WriteString("PlayerCharacter");
            buffer.WriteString(player->GetPlayerName());
            buffer.WriteVector3(pos.X, pos.Y, pos.Z);
            _netClient->SendPacket(0, Networking::PacketType::PlayerJoin, buffer);

            // Événement local
            Events::EventManager::Get().TriggerEvent("playerSpawned");
        }
    }

    void SyncLocalPlayer() {
        auto& entityMgr = Entities::EntityManager::Get();
        auto* player = entityMgr.GetEntity(_localPlayerId);

        if (!player || !player->IsDirty()) return;

        // Récupérer la position du jeu
        FVector gamePos = Natives::GetPlayerCoords();
        player->SetPosition(gamePos);

        // Envoyer au serveur
        Networking::NetBuffer buffer;
        buffer.WriteUInt32(_localPlayerId);
        buffer.WriteVector3(gamePos.X, gamePos.Y, gamePos.Z);
        _netClient->SendPacket(0, Networking::PacketType::PlayerUpdate, buffer);

        player->ClearDirty();
    }

    bool _running = true;
    uint32_t _localPlayerId = 0;
    std::unique_ptr<Networking::NetLibraryClient> _netClient;
};

// ============================================================================
// GLOBAL CLIENT INSTANCE
// ============================================================================

std::unique_ptr<HogwartsMPClient> gClient = nullptr;

// ============================================================================
// DLL ENTRY POINT
// ============================================================================

DWORD WINAPI ClientThread(LPVOID lpParam) {
    // Créer le client
    gClient = std::make_unique<HogwartsMPClient>();

    if (!gClient->Initialize()) {
        DEBUG_LOG(Utilities::DebugCategory::None,
                  Utilities::DebugLevel::Basic,
                  "Failed to initialize client");
        return 1;
    }

    // Connexion optionnelle au serveur
    // gClient->Connect();

    // Main loop
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (gClient->IsRunning()) {
        auto now = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;

        gClient->Update(deltaTime);

        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
    }

    gClient->Shutdown();
    gClient.reset();

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hModule);

            // Allocate console for debug output
            AllocConsole();
            SetConsoleTitleW(L"HogwartsMP Client v2.0 - Debug Console");

            // Redirect stdout/stderr to console
            FILE* fDummy;
            freopen_s(&fDummy, "CONOUT$", "w", stdout);
            freopen_s(&fDummy, "CONOUT$", "w", stderr);

            // Start client thread
            CreateThread(nullptr, 0, ClientThread, nullptr, 0, nullptr);
            break;

        case DLL_PROCESS_DETACH:
            if (gClient) {
                gClient->Stop();
            }
            break;
    }

    return TRUE;
}
