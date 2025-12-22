#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <queue>
#include "../../../shared/networking/network_packet.h"

// Forward declare ENet types
struct _ENetHost;
struct _ENetPeer;
typedef _ENetHost ENetHost;
typedef _ENetPeer ENetPeer;

namespace HogwartsMP {
namespace Networking {

    // Define callback types
    using PacketCallback = std::function<void(uint32_t clientId, PacketType, const uint8_t*, size_t)>;
    using ClientConnectCallback = std::function<void(uint32_t clientId)>;
    using ClientDisconnectCallback = std::function<void(uint32_t clientId)>;

    struct ConnectedClient {
        uint32_t id;
        ENetPeer* peer;
        std::string ipAddress;
        uint16_t port;
        uint64_t connectTime;
        bool handshakeComplete;
    };

    class NetworkServer {
    public:
        NetworkServer();
        ~NetworkServer();

        // Prevent copying
        NetworkServer(const NetworkServer&) = delete;
        NetworkServer& operator=(const NetworkServer&) = delete;

        // Server management
        bool Start(uint16_t port, size_t maxClients = 32);
        void Stop();
        bool IsRunning() const { return _running; }

        // Main loop update
        void Update();

        // Sending data
        bool SendPacket(uint32_t clientId, const NetworkPacket& packet, bool reliable = true);
        bool SendRaw(uint32_t clientId, const std::vector<uint8_t>& data, bool reliable = true);
        
        void BroadcastPacket(const NetworkPacket& packet, bool reliable = true, uint32_t excludeClientId = 0);
        void BroadcastRaw(const std::vector<uint8_t>& data, bool reliable = true, uint32_t excludeClientId = 0);

        // Client management
        void DisconnectClient(uint32_t clientId, const std::string& reason = "Server initiated disconnect");
        ConnectedClient* GetClient(uint32_t clientId);
        std::vector<ConnectedClient*> GetAllClients();
        size_t GetClientCount() const;
        size_t GetMaxClients() const { return _maxClients; }

        // Callbacks registration
        void SetOnClientConnected(ClientConnectCallback callback) { _onClientConnected = callback; }
        void SetOnClientDisconnected(ClientDisconnectCallback callback) { _onClientDisconnected = callback; }
        void SetOnPacketReceived(PacketCallback callback) { _onPacketReceived = callback; }

        // Statistics
        uint16_t GetPort() const { return _port; }
        uint64_t GetBytesSent() const { return _bytesSent; }
        uint64_t GetBytesReceived() const { return _bytesReceived; }

    private:
        // Internal ENet handling
        void HandleConnect(ENetPeer* peer);
        void HandleDisconnect(ENetPeer* peer);
        void HandleReceive(ENetPeer* peer, const uint8_t* data, size_t size);
        
        uint32_t GenerateClientId();
        std::string GetPeerIP(ENetPeer* peer);

        ENetHost* _host = nullptr;
        bool _running = false;
        uint16_t _port = 0;
        size_t _maxClients = 32;

        // Client storage
        // Maps ClientID -> Client Data
        std::unordered_map<uint32_t, ConnectedClient> _clients;
        // Maps ENetPeer* -> ClientID (for quick lookup)
        std::unordered_map<ENetPeer*, uint32_t> _peerToClientId;
        
        uint32_t _nextClientId = 1;

        // Callbacks
        ClientConnectCallback _onClientConnected;
        ClientDisconnectCallback _onClientDisconnected;
        PacketCallback _onPacketReceived;

        // Stats
        uint64_t _bytesSent = 0;
        uint64_t _bytesReceived = 0;
    };

} // namespace Networking
} // namespace HogwartsMP
