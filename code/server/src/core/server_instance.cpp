#include "server_instance.h"
#include "../../../shared/logging/logger.h"
#include "../networking/network_server.h"

namespace HogwartsMP::Core {

// Constructor and destructor defined here to allow forward declaration in header
ServerInstance::ServerInstance() = default;
ServerInstance::~ServerInstance() = default;

bool ServerInstance::Init(ServerOptions opts) {
    if (_initialized) {
        Logging::Logger::Warning("ServerInstance::Init called but already initialized");
        return false;
    }

    _options = opts;

    Logging::Logger::InfoF("Initializing ServerInstance: %s v%s",
                          _options.serverName.c_str(),
                          _options.gameVersion.c_str());
    Logging::Logger::InfoF("Port: %d, WebPort: %d, MaxPlayers: %zu",
                          _options.port,
                          _options.webPort,
                          _options.maxPlayers);

    // Initialize ECS world
    _world = std::make_unique<flecs::world>();
    if (!_world) {
        Logging::Logger::Error("Failed to create ECS world");
        return false;
    }

    Logging::Logger::Info("ECS World created successfully");

    // Initialize network server
    _networkServer = std::make_unique<Networking::NetworkServer>();
    if (!_networkServer->Start(_options.port, _options.maxPlayers)) {
        Logging::Logger::Error("Failed to start NetworkServer");
        return false;
    }

    Logging::Logger::InfoF("NetworkServer started on port %d", _options.port);

    // Call derived class initialization
    if (!PostInit()) {
        Logging::Logger::Error("PostInit failed in derived class");
        return false;
    }

    _initialized = true;
    Logging::Logger::Info("ServerInstance initialized successfully");

    return true;
}

void ServerInstance::Update() {
    if (!_initialized) {
        return;
    }

    // Update network server
    if (_networkServer) {
        _networkServer->Update();
    }

    // Update ECS world
    if (_world) {
        _world->progress();
    }

    // Call derived class update
    PostUpdate();
}

void ServerInstance::Shutdown() {
    if (!_initialized) {
        return;
    }

    Logging::Logger::Info("Shutting down ServerInstance");

    // Call derived class shutdown
    if (!PreShutdown()) {
        Logging::Logger::Warning("PreShutdown returned false");
    }

    // Stop network server
    if (_networkServer) {
        _networkServer->Stop();
        _networkServer.reset();
        Logging::Logger::Info("NetworkServer stopped and destroyed");
    }

    // Cleanup ECS world
    if (_world) {
        _world.reset();
        Logging::Logger::Info("ECS World destroyed");
    }

    _initialized = false;
    Logging::Logger::Info("ServerInstance shutdown complete");
}

} // namespace HogwartsMP::Core
