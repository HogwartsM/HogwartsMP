#include "network_server.h"
#include "../../../shared/logging/logger.h"
#include "../logging/network_logger.h"
#include <enet/enet.h>
#include <ctime>
#include <sstream>

namespace HogwartsMP::Networking {

static std::string PacketTypeToString(PacketType type) {
    switch (type) {
        case PacketType::Connect: return "CONNECT";
        case PacketType::Disconnect: return "DISCONNECT";
        case PacketType::ChatMessage: return "CHAT_MESSAGE";
        case PacketType::PlayerJoin: return "PLAYER_JOIN";
        case PacketType::PlayerLeave: return "PLAYER_LEAVE";
        case PacketType::PlayerUpdate: return "PLAYER_UPDATE";
        case PacketType::Event: return "EVENT";
        case PacketType::RPC: return "RPC";
        case PacketType::ResourceList: return "RES_LIST";
        case PacketType::ResourceRequest: return "RES_REQUEST";
        case PacketType::ResourceManifest: return "RES_MANIFEST";
        case PacketType::ResourceFile: return "RES_FILE";
        case PacketType::ResourceComplete: return "RES_COMPLETE";
        case PacketType::Custom: return "CUSTOM";
        default: return "UNKNOWN_" + std::to_string(static_cast<int>(type));
    }
}

NetworkServer::NetworkServer() {
    // Initialize ENet
    if (enet_initialize() != 0) {
        Logging::Logger::Error("Failed to initialize ENet");
    } else {
        Logging::Logger::Info("ENet initialized successfully");
    }
}

NetworkServer::~NetworkServer() {
    Stop();
    enet_deinitialize();
}

bool NetworkServer::Start(uint16_t port, size_t maxClients) {
    if (_running) {
        Logging::Logger::Warning("Server is already running");
        return false;
    }

    _port = port;
    _maxClients = maxClients;

    // Create server
    ENetAddress address;
    address.host = ENET_HOST_ANY;
    address.port = port;

    _server = enet_host_create(
        &address,           // address to bind
        maxClients,         // max clients
        32,                 // channels
        0,                  // incoming bandwidth (unlimited)
        0                   // outgoing bandwidth (unlimited)
    );

    if (_server == nullptr) {
        Logging::Logger::ErrorF("Failed to create ENet server on port %d", port);
        return false;
    }

    _running = true;
    Logging::Logger::InfoF("Server started on port %d (max clients: %zu)", port, maxClients);

    return true;
}

void NetworkServer::Stop() {
    if (!_running) {
        return;
    }

    Logging::Logger::Info("Stopping server...");
    Logging::NetworkLogger::Get().Shutdown();

    // Disconnect all clients
    for (auto& [id, client] : _clients) {
        if (client.peer) {
            enet_peer_disconnect(client.peer, 0);
        }
    }

    // Wait a bit for disconnects
    ENetEvent event;
    while (enet_host_service(_server, &event, 1000) > 0) {
        if (event.type == ENET_EVENT_TYPE_RECEIVE) {
            enet_packet_destroy(event.packet);
        }
    }

    // Cleanup
    _clients.clear();
    _peerToClient.clear();

    if (_server) {
        enet_host_destroy(_server);
        _server = nullptr;
    }

    _running = false;
    Logging::Logger::Info("Server stopped");
}

void NetworkServer::Update() {
    if (!_running || !_server) {
        return;
    }

    ENetEvent event;
    while (enet_host_service(_server, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                OnClientConnect(event.peer);
                break;

            case ENET_EVENT_TYPE_RECEIVE: {
                auto it = _peerToClient.find(event.peer);
                if (it != _peerToClient.end()) {
                    OnReceive(it->second, event.packet->data, event.packet->dataLength);
                    _bytesReceived += event.packet->dataLength;
                }
                enet_packet_destroy(event.packet);
                break;
            }

            case ENET_EVENT_TYPE_DISCONNECT: {
                auto it = _peerToClient.find(event.peer);
                if (it != _peerToClient.end()) {
                    OnClientDisconnect(it->second);
                }
                break;
            }

            default:
                break;
        }
    }
}

bool NetworkServer::SendPacket(uint32_t clientId, const NetworkPacket& packet, bool reliable) {
    auto data = packet.Serialize();
    return SendRaw(clientId, data, reliable);
}

bool NetworkServer::SendRaw(uint32_t clientId, const std::vector<uint8_t>& data, bool reliable) {
    auto it = _clients.find(clientId);
    if (it == _clients.end() || !it->second.peer) {
        Logging::Logger::WarningF("Cannot send packet: client %d not found", clientId);
        return false;
    }

    uint32_t flags = reliable ? ENET_PACKET_FLAG_RELIABLE : 0;

    ENetPacket* packet = enet_packet_create(
        data.data(),
        data.size(),
        flags
    );

    if (packet == nullptr) {
        Logging::Logger::Error("Failed to create ENet packet");
        return false;
    }

    if (enet_peer_send(it->second.peer, 0, packet) < 0) {
        Logging::Logger::ErrorF("Failed to send packet to client %d", clientId);
        return false;
    }

    _bytesSent += data.size();
    return true;
}

void NetworkServer::BroadcastPacket(const NetworkPacket& packet, bool reliable) {
    auto data = packet.Serialize();
    BroadcastRaw(data, reliable);
}

void NetworkServer::BroadcastRaw(const std::vector<uint8_t>& data, bool reliable) {
    for (auto& [id, client] : _clients) {
        SendRaw(id, data, reliable);
    }
}

void NetworkServer::DisconnectClient(uint32_t clientId) {
    auto it = _clients.find(clientId);
    if (it == _clients.end()) {
        return;
    }

    if (it->second.peer) {
        enet_peer_disconnect(it->second.peer, 0);
    }

    OnClientDisconnect(clientId);
}

ConnectedClient* NetworkServer::GetClient(uint32_t clientId) {
    auto it = _clients.find(clientId);
    if (it != _clients.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<ConnectedClient*> NetworkServer::GetAllClients() {
    std::vector<ConnectedClient*> result;
    result.reserve(_clients.size());

    for (auto& [id, client] : _clients) {
        result.push_back(&client);
    }

    return result;
}

uint32_t NetworkServer::GenerateClientId() {
    return _nextClientId++;
}

void NetworkServer::OnClientConnect(ENetPeer* peer) {
    uint32_t clientId = GenerateClientId();

    char ipString[64];
    enet_address_get_host_ip(&peer->address, ipString, sizeof(ipString));

    ConnectedClient client;
    client.id = clientId;
    client.peer = peer;
    client.ipAddress = ipString;
    client.port = peer->address.port;
    client.connectTime = std::time(nullptr);

    _clients[clientId] = client;
    _peerToClient[peer] = clientId;

    Logging::Logger::InfoF("Client %d connected from %s:%d",
                          clientId, ipString, peer->address.port);

    Logging::NetworkLogger::Get().LogConnection(ipString);

    if (_onClientConnected) {
        _onClientConnected(clientId);
    }
}

void NetworkServer::OnClientDisconnect(uint32_t clientId) {
    auto it = _clients.find(clientId);
    if (it == _clients.end()) {
        return;
    }

    Logging::Logger::InfoF("Client %d disconnected", clientId);

    if (it->second.peer) {
        _peerToClient.erase(it->second.peer);
    }

    Logging::NetworkLogger::Get().LogDisconnection(it->second.ipAddress);

    _clients.erase(it);

    if (_onClientDisconnected) {
        _onClientDisconnected(clientId);
    }
}

void NetworkServer::OnReceive(uint32_t clientId, const uint8_t* data, size_t size) {
    if (size < 1) {
        return;
    }

    // First byte is packet type
    PacketType type = static_cast<PacketType>(data[0]);

    std::string clientIp = "unknown";
    auto it = _clients.find(clientId);
    if (it != _clients.end()) {
        clientIp = it->second.ipAddress;
    }

    Logging::NetworkLogger::Get().LogRequest(clientIp, PacketTypeToString(type), "/game/packet", size);

    if (_onPacketReceived) {
        _onPacketReceived(clientId, type, data + 1, size - 1);
    }
}

} // namespace HogwartsMP::Networking
