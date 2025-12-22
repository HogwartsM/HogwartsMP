#include "server.h"
#include "../../../shared/logging/logger.h"
#include "../networking/network_server.h"
#include <vector>

namespace HogwartsMP {

    // Constructor and destructor defined here to allow proper cleanup of base class
    Server::Server() = default;
    Server::~Server() = default;

    bool Server::PostInit() {
        _serverRef = this;

        Logging::Logger::Info("Server::PostInit called");

        // Initialize CLI
        _cli = std::make_unique<Core::CLI>();
        InitCommands();
        _cli->Start();

        // TODO: Register ECS modules (Mod, HumanSync, Human)
        // TODO: Setup default weather/time components
        // TODO: Setup player connect/disconnect callbacks

        InitNetworkingMessages();

        Logging::Logger::Info("Server PostInit complete");
        return true;
    }

    void Server::PostUpdate() {
        // TODO: Update server logic
        // TODO: Process player updates
    }

    bool Server::PreShutdown() {
        Logging::Logger::Info("Server::PreShutdown called");

        if (_cli) {
            _cli->Stop();
            _cli.reset();
        }

        // TODO: Cleanup server resources
        // TODO: Disconnect all players

        _serverRef = nullptr;
        return true;
    }

    void Server::InitNetworkingMessages() {
        Logging::Logger::Info("Initializing networking messages...");

        auto* networkServer = GetNetworkServer();
        if (!networkServer) {
            Logging::Logger::Error("Failed to get NetworkServer");
            return;
        }

        // Register connect callback
        networkServer->SetOnClientConnected([this](uint32_t clientId) {
            Logging::Logger::InfoF("Server::OnClientConnected - Client %d", clientId);
            
            // Send a welcome message immediately upon connection to verify path
            std::string welcome = "Connection Established. Welcome to HogwartsMP!";
            
            // We need to implement proper serialization, but for now sending raw bytes
            std::vector<uint8_t> packetData;
            packetData.push_back(static_cast<uint8_t>(Networking::PacketType::ChatMessage));
            packetData.insert(packetData.end(), welcome.begin(), welcome.end());
            
            GetNetworkServer()->SendRaw(clientId, packetData, true);

            // TODO: Create player entity
        });

        // Register disconnect callback
        networkServer->SetOnClientDisconnected([this](uint32_t clientId) {
            Logging::Logger::InfoF("Server::OnClientDisconnected - Client %d", clientId);

            // TODO: Destroy player entity
        });

        // Register packet handler
        networkServer->SetOnPacketReceived([this](uint32_t clientId, Networking::PacketType type, const uint8_t* data, size_t size) {
            // Handle packet
            switch (type) {
                case Networking::PacketType::ChatMessage: {
                    // Simple chat handling for now
                    std::string message(reinterpret_cast<const char*>(data), size);
                    BroadcastChatMessage(message);
                    break;
                }
                case Networking::PacketType::Connect: {
                    std::string msg(reinterpret_cast<const char*>(data), size);
                    Logging::Logger::InfoF("Client %d handshake: %s", clientId, msg.c_str());
                    
                    // Reply with welcome message
                    std::string welcome = "Welcome to HogwartsMP!";
                    BroadcastChatMessage(welcome); 
                    break;
                }
                default:
                    break;
            }
        });

        InitRPCs();

        Logging::Logger::Info("Networking messages registered");
    }

    void Server::BroadcastChatMessage(const std::string &msg) {
        Logging::Logger::InfoF("Broadcast: %s", msg.c_str());

        auto* networkServer = GetNetworkServer();
        if (networkServer) {
            // We need to implement proper serialization, but for now sending raw bytes
            // Manually construct a packet for ChatMessage (packet ID 40)
            // Ideally we should use NetworkPacket class but it's abstract or not fully linked here?
            // Let's use BroadcastRaw directly with packet ID prepended if needed, 
            // BUT NetworkServer::BroadcastRaw just sends bytes.
            // Wait, NetworkPacket is in shared/networking/network_packet.h
            // We should use a concrete implementation or just raw bytes if the protocol expects it.
            // Let's assume the first byte is the packet ID as per NetworkServer::OnReceive
            
            std::vector<uint8_t> packetData;
            packetData.push_back(static_cast<uint8_t>(Networking::PacketType::ChatMessage));
            packetData.insert(packetData.end(), msg.begin(), msg.end());
            
            networkServer->BroadcastRaw(packetData, true);
        }
    }

    void Server::InitRPCs() {
        Logging::Logger::Info("Initializing RPCs (placeholder)");

        // TODO: Register RPC handlers (chat, commands, etc.)
    }

    void Server::InitCommands() {
        if (!_cli) return;

        _cli->RegisterCommand("players", [this](const std::vector<std::string>& args) {
            auto* networkServer = GetNetworkServer();
            if (networkServer) {
                size_t count = networkServer->GetClientCount();
                size_t max = networkServer->GetMaxClients();
                Logging::Logger::InfoF("Players connected: %zu / %zu", count, max);
                
                auto clients = networkServer->GetAllClients();
                if (!clients.empty()) {
                    Logging::Logger::Info("Connected clients:");
                    for (const auto* client : clients) {
                        Logging::Logger::InfoF(" - ID: %d, IP: %s", client->id, client->ipAddress.c_str());
                    }
                }
            } else {
                Logging::Logger::Error("NetworkServer not available");
            }
        }, "Show connected players count and list");

        _cli->RegisterCommand("stop", [this](const std::vector<std::string>& args) {
            Logging::Logger::Info("Stopping server from CLI...");
            // In a real application, we should signal the main loop to exit.
            // Since we don't have a clean way to signal main loop yet (it's infinite), 
            // we will force exit for now, or we could set a flag if we had access to one.
            // Ideally, ServerInstance should have a 'IsRunning' flag we can toggle.
            // But main.cpp has `while(true)`. 
            // Let's just call exit(0) for now as it's a CLI command.
            Shutdown();
            exit(0);
        }, "Stop the server");

        _cli->RegisterCommand("help", [this](const std::vector<std::string>& args) {
            Logging::Logger::Info("Available commands:");
            Logging::Logger::Info("  players - Show connected players");
            Logging::Logger::Info("  stop    - Stop the server");
            Logging::Logger::Info("  help    - Show this help message");
        }, "Show help message");
    }

} // namespace HogwartsMP
