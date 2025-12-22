#include "client_instance.h"
#include "../../../shared/logging/logger.h"
#include "../networking/network_client.h"

namespace HogwartsMP::Core {

// Constructor and destructor defined here to allow forward declaration in header
ClientInstance::ClientInstance() = default;
ClientInstance::~ClientInstance() = default;

bool ClientInstance::Init(ClientOptions opts) {
    if (_initialized) {
        Logging::Logger::Warning("ClientInstance::Init called but already initialized");
        return false;
    }

    _options = opts;

    Logging::Logger::InfoF("Initializing ClientInstance for %s v%s",
                          _options.gameName.c_str(),
                          _options.gameVersion.c_str());

    // Initialize ECS world
    _world = std::make_unique<flecs::world>();
    if (!_world) {
        Logging::Logger::Error("Failed to create ECS world");
        return false;
    }

    Logging::Logger::Info("ECS World created successfully");

    // Initialize network client
    _networkClient = std::make_unique<Networking::NetworkClient>();
    Logging::Logger::Info("NetworkClient created successfully");

    // Call derived class initialization
    if (!PostInit()) {
        Logging::Logger::Error("PostInit failed in derived class");
        return false;
    }

    _initialized = true;
    Logging::Logger::Info("ClientInstance initialized successfully");

    return true;
}

void ClientInstance::Update() {
    if (!_initialized) {
        return;
    }

    // Update network client
    if (_networkClient) {
        _networkClient->Update();
    }

    // Update ECS world
    if (_world) {
        _world->progress();
    }

    // Call derived class update
    PostUpdate();
}

void ClientInstance::Shutdown() {
    if (!_initialized) {
        return;
    }

    Logging::Logger::Info("Shutting down ClientInstance");

    // Call derived class shutdown
    if (!PreShutdown()) {
        Logging::Logger::Warning("PreShutdown returned false");
    }

    // Disconnect network client
    if (_networkClient) {
        _networkClient->Disconnect();
        _networkClient.reset();
        Logging::Logger::Info("NetworkClient destroyed");
    }

    // Cleanup ECS world
    if (_world) {
        _world.reset();
        Logging::Logger::Info("ECS World destroyed");
    }

    _initialized = false;
    Logging::Logger::Info("ClientInstance shutdown complete");
}

} // namespace HogwartsMP::Core
