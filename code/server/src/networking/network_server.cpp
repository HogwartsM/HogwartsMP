#include "network_server.h"
#include "../../../shared/logging/logger.h"
#include <enet/enet.h>
#include <iostream>
#include <algorithm>

namespace HogwartsMP::Networking {

    NetworkServer::NetworkServer() {
        if (enet_initialize() != 0) {
            Logging::Logger::Error("An error occurred while initializing ENet.");
        } else {
            Logging::Logger::Info("ENet initialized successfully.");
        }
    }

    NetworkServer::~NetworkServer() {
        Stop();
        enet_deinitialize();
    }

    bool NetworkServer::Start(uint16_t port, size_t maxClients) {
        if (_running) {
            Logging::Logger::Warning("Server is already running.");
            return false;
        }

        _port = port;
        _maxClients = maxClients;

        ENetAddress address;
        address.host = ENET_HOST_ANY;
        address.port = port;

        // Create the server host
        _host = enet_host_create(&address, maxClients, 2, 0, 0);

        if (_host == nullptr) {
            Logging::Logger::Error("An error occurred while trying to create an ENet server host.");
            return false;
        }

        _running = true;
        Logging::Logger::InfoF("NetworkServer started on port %d with max %zu clients.", port, maxClients);
        return true;
    }

    void NetworkServer::Stop() {
        if (!_running) return;

        Logging::Logger::Info("Stopping NetworkServer...");

        // Disconnect all peers
        for (auto& pair : _clients) {
            if (pair.second.peer) {
                enet_peer_disconnect(pair.second.peer, 0);
            }
        }

        // Allow ENet to send disconnect packets
        if (_host) {
            enet_host_flush(_host);
            // Wait briefly for disconnects to go out? 
            // Usually we pump the host for a bit, but here we just destroy it.
            enet_host_destroy(_host);
            _host = nullptr;
        }

        _clients.clear();
        _peerToClientId.clear();
        _running = false;
        Logging::Logger::Info("NetworkServer stopped.");
    }

    void NetworkServer::Update() {
        if (!_running || !_host) return;

        ENetEvent event;
        // Check for events
        while (enet_host_service(_host, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    HandleConnect(event.peer);
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    HandleReceive(event.peer, event.packet->data, event.packet->dataLength);
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    HandleDisconnect(event.peer);
                    break;
                default:
                    break;
            }
        }
    }

    bool NetworkServer::SendPacket(uint32_t clientId, const NetworkPacket& packet, bool reliable) {
        std::vector<uint8_t> data = packet.Serialize();
        return SendRaw(clientId, data, reliable);
    }

    bool NetworkServer::SendRaw(uint32_t clientId, const std::vector<uint8_t>& data, bool reliable) {
        auto it = _clients.find(clientId);
        if (it == _clients.end()) {
            Logging::Logger::ErrorF("SendRaw: Client %d not found.", clientId);
            return false;
        }

        ENetPeer* peer = it->second.peer;
        if (!peer) return false;

        ENetPacket* packet = enet_packet_create(data.data(), data.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
        if (enet_peer_send(peer, 0, packet) < 0) {
            Logging::Logger::ErrorF("Failed to send packet to client %d.", clientId);
            return false;
        }

        _bytesSent += data.size();
        return true;
    }

    void NetworkServer::BroadcastPacket(const NetworkPacket& packet, bool reliable, uint32_t excludeClientId) {
        std::vector<uint8_t> data = packet.Serialize();
        BroadcastRaw(data, reliable, excludeClientId);
    }

    void NetworkServer::BroadcastRaw(const std::vector<uint8_t>& data, bool reliable, uint32_t excludeClientId) {
        ENetPacket* packet = enet_packet_create(data.data(), data.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
        
        // We can use enet_host_broadcast but it doesn't support exclusion easily without logic
        // But enet_host_broadcast is efficient.
        // If excludeClientId is 0 (invalid), use built-in broadcast
        if (excludeClientId == 0 && _host) {
             enet_host_broadcast(_host, 0, packet);
             _bytesSent += (data.size() * _clients.size()); // Approximate
        } else {
            // Manual broadcast
            // Note: enet_packet_create allocates, but enet_peer_send takes ownership?
            // Wait, enet_peer_send does NOT take ownership if we send to multiple peers we need to be careful?
            // Actually enet_host_broadcast takes the packet.
            // If we send manually, we should create packet once?
            // enet_peer_send: "packet packet to send"
            // If we send the same packet to multiple peers, we might need to increment reference count?
            // ENet docs say: "packets may be sent to multiple peers... packet reference count will be incremented"
            
            // So we can reuse the packet pointer.
            for (auto& pair : _clients) {
                if (pair.first == excludeClientId) continue;
                enet_peer_send(pair.second.peer, 0, packet);
                _bytesSent += data.size();
            }
            
            // If we didn't use broadcast, we need to handle the packet lifecycle?
            // enet_packet_create returns a packet with refCount 0 (initial).
            // enet_peer_send increments it.
            // If we send to N peers, refCount becomes N.
            // When peers dispatch, they decrement.
            // BUT: if we created it and didn't broadcast, we might need to check if we need to destroy it if N=0?
            // Actually, if we send it at least once, ENet owns it.
            // If we don't send it at all (e.g. no clients), we must destroy it.
            
            if (_clients.empty() || (_clients.size() == 1 && _clients.begin()->first == excludeClientId)) {
                 enet_packet_destroy(packet);
            }
        }
    }

    void NetworkServer::DisconnectClient(uint32_t clientId, const std::string& reason) {
        auto it = _clients.find(clientId);
        if (it != _clients.end()) {
            Logging::Logger::InfoF("Disconnecting client %d: %s", clientId, reason.c_str());
            enet_peer_disconnect(it->second.peer, 0);
            // The actual cleanup happens in HandleDisconnect when ENet processes the event
        }
    }

    ConnectedClient* NetworkServer::GetClient(uint32_t clientId) {
        auto it = _clients.find(clientId);
        if (it != _clients.end()) {
            return &it->second;
        }
        return nullptr;
    }

    std::vector<ConnectedClient*> NetworkServer::GetAllClients() {
        std::vector<ConnectedClient*> clients;
        clients.reserve(_clients.size());
        for (auto& pair : _clients) {
            clients.push_back(&pair.second);
        }
        return clients;
    }
    
    size_t NetworkServer::GetClientCount() const {
        return _clients.size();
    }

    // --- Internal handlers ---

    void NetworkServer::HandleConnect(ENetPeer* peer) {
        uint32_t id = GenerateClientId();
        std::string ip = GetPeerIP(peer);

        Logging::Logger::InfoF("Client connecting from %s (Assigned ID: %d)", ip.c_str(), id);

        ConnectedClient newClient;
        newClient.id = id;
        newClient.peer = peer;
        newClient.ipAddress = ip;
        newClient.port = peer->address.port;
        newClient.connectTime = std::time(nullptr); // Simple timestamp
        newClient.handshakeComplete = false;

        _clients[id] = newClient;
        _peerToClientId[peer] = id;

        // Store the ID in the peer's data field for easy access if needed
        peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(id));

        if (_onClientConnected) {
            _onClientConnected(id);
        }
    }

    void NetworkServer::HandleDisconnect(ENetPeer* peer) {
        auto it = _peerToClientId.find(peer);
        if (it == _peerToClientId.end()) {
            return;
        }

        uint32_t id = it->second;
        Logging::Logger::InfoF("Client %d disconnected.", id);

        if (_onClientDisconnected) {
            _onClientDisconnected(id);
        }

        _clients.erase(id);
        _peerToClientId.erase(it);
        
        peer->data = nullptr;
    }

    void NetworkServer::HandleReceive(ENetPeer* peer, const uint8_t* data, size_t size) {
        auto it = _peerToClientId.find(peer);
        if (it == _peerToClientId.end()) {
            return;
        }

        uint32_t id = it->second;
        _bytesReceived += size;

        // Basic validation
        if (size < 1) return;

        PacketType type = static_cast<PacketType>(data[0]);
        // Logging::Logger::DebugF("Received packet type %d from client %d (size %zu)", static_cast<int>(type), id, size);

        if (_onPacketReceived) {
            _onPacketReceived(id, type, data, size);
        }
    }

    uint32_t NetworkServer::GenerateClientId() {
        return _nextClientId++;
    }

    std::string NetworkServer::GetPeerIP(ENetPeer* peer) {
        char buffer[64];
        enet_address_get_host_ip(&peer->address, buffer, sizeof(buffer));
        return std::string(buffer);
    }

} // namespace HogwartsMP::Networking
