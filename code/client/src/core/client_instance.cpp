#include "client_instance.h"
#include "../../../shared/logging/logger.h"
#include "../Services/network_client.h"
#include <vector>
#include <windows.h>

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

    _networkClient->SetOnPacketReceived([](Networking::PacketType type, const uint8_t* data, size_t size) {
        Logging::Logger::InfoF("Packet received: Type %d, Size %zu", static_cast<int>(type), size);
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
