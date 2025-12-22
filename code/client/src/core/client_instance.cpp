#include <vector>
#include <windows.h>
#include "client_instance.h"
#include "../../../shared/logging/logger.h"
#include "../Services/network_client.h"
#include "../Overlay/overlay_manager.h"
#include "../Overlay/Widgets/chat_widget.h"
#include "../Overlay/Widgets/stats_widget.h"

namespace HogwartsMP::Core {

// Constructor and destructor defined here to allow forward declaration in header
ClientInstance::ClientInstance() = default;
ClientInstance::~ClientInstance() = default;

/**
 * @brief Initializes the client instance with the given options.
 *
 * This method sets up the game environment, ECS world, and network client.
 * It ensures that initialization happens only once.
 *
 * @param opts The configuration options for the client.
 * @return true if initialization is successful, false otherwise.
 */
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

    // Register callbacks
    _networkClient->SetOnConnected([this]() {
        Logging::Logger::Info("ClientInstance::OnConnected");
        
        // Send handshake packet
        std::string handshakeMsg = "Hello from Client!";
        std::vector<uint8_t> packetData;
        packetData.push_back(static_cast<uint8_t>(Networking::PacketType::Connect)); // 0 = Connect
        packetData.insert(packetData.end(), handshakeMsg.begin(), handshakeMsg.end());
        
        // Send reliable
        _networkClient->SendRaw(packetData, true);
    });

    _networkClient->SetOnDisconnected([this]() {
        Logging::Logger::Info("ClientInstance::OnDisconnected");
    });

    _networkClient->SetOnPacketReceived([this](Networking::PacketType type, const uint8_t* data, size_t size) {
        // Logging::Logger::InfoF("Packet received: Type %d, Size %zu", static_cast<int>(type), size);
        
        // Note: Packet routing is now handled in InitOverlay() for Chat
        // Future packets will be routed to other subsystems here
    });

    // Connect to server
    if (!_networkClient->Connect(_options.serverHost, _options.serverPort)) {
        Logging::Logger::ErrorF("Failed to connect to server at %s:%d", _options.serverHost.c_str(), _options.serverPort);
        MessageBoxA(NULL, "Failed to connect to server. The game will now close.", "HogwartsMP Error", MB_OK | MB_ICONERROR);
        TerminateProcess(GetCurrentProcess(), 1);
        return false;
    }
    
    Logging::Logger::InfoF("Connected to server at %s:%d", _options.serverHost.c_str(), _options.serverPort);

    // Initialize Overlay
    InitOverlay();

    // Call derived class initialization
    if (!PostInit()) {
        Logging::Logger::Error("PostInit failed in derived class");
        return false;
    }

    _initialized = true;
    Logging::Logger::Info("ClientInstance initialized successfully");

    return true;
}

/**
 * @brief Updates the client instance.
 *
 * This method is called every frame to update the network client,
 * the ECS world, and any derived class logic.
 */
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

    // Update Overlay
    UpdateOverlay();

    // Call derived class update
    PostUpdate();
}

void ClientInstance::InitOverlay() {
    auto& overlay = Overlay::OverlayManager::Get();
    // overlay.Init(); // Moved to Render Hook (deferred init)

    // Add default widgets
    auto chat = std::make_shared<Overlay::Widgets::ChatWidget>();
    
    // Bind network send to chat
    chat->SetOnMessageSentCallback([this](const std::string& msg) {
        if (_networkClient && _networkClient->IsConnected()) {
            std::vector<uint8_t> packetData;
            packetData.push_back(static_cast<uint8_t>(Networking::PacketType::ChatMessage));
            packetData.insert(packetData.end(), msg.begin(), msg.end());
            _networkClient->SendRaw(packetData, true);
        }
    });

    overlay.AddWidget(chat);
    
    // Connect network receive to chat
    _networkClient->SetOnPacketReceived([this, chat](Networking::PacketType type, const uint8_t* data, size_t size) {
        if (type == Networking::PacketType::ChatMessage) {
            std::string msg(reinterpret_cast<const char*>(data), size);
            chat->AddMessage(msg);
            
            // Also notify via Overlay
            Overlay::OverlayManager::Get().Notify(msg);
        }
    });
}

void ClientInstance::UpdateOverlay() {
    Overlay::OverlayManager::Get().Update();
}

void ClientInstance::RenderOverlay() {
    Overlay::OverlayManager::Get().Render();
}

/**
 * @brief Shuts down the client instance.
 *
 * This method cleans up resources, disconnects the network client,
 * and destroys the ECS world. It ensures a graceful shutdown.
 */
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
