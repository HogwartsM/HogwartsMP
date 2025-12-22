#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include "../../../shared/networking/network_packet.h"

// Forward declare ENet types
struct _ENetHost;
struct _ENetPeer;
typedef _ENetHost ENetHost;
typedef _ENetPeer ENetPeer;

namespace HogwartsMP::Networking {

using PacketCallback = std::function<void(uint32_t clientId, PacketType, const uint8_t*, size_t)>;
using ClientConnectCallback = std::function<void(uint32_t clientId)>;
using ClientDisconnectCallback = std::function<void(uint32_t clientId)>;

struct ConnectedClient {
    uint32_t id;
    ENetPeer* peer;
    std::string ipAddress;
    uint16_t port;
    uint64_t connectTime;
};

class NetworkServer {
public:
    NetworkServer();
    ~NetworkServer();

    // Server management
    bool Start(uint16_t port, size_t maxClients = 512);
    void Stop();
    bool IsRunning() const { return _running; }

    // Update (call every frame)
    void Update();

    // Send packets
    bool SendPacket(uint32_t clientId, const NetworkPacket& packet, bool reliable = true);
    bool SendRaw(uint32_t clientId, const std::vector<uint8_t>& data, bool reliable = true);
    void BroadcastPacket(const NetworkPacket& packet, bool reliable = true);
    void BroadcastRaw(const std::vector<uint8_t>& data, bool reliable = true);

    // Client management
    void DisconnectClient(uint32_t clientId);
    ConnectedClient* GetClient(uint32_t clientId);
    std::vector<ConnectedClient*> GetAllClients();
    size_t GetClientCount() const { return _clients.size(); }

    // Callbacks
    void SetOnClientConnected(ClientConnectCallback callback) { _onClientConnected = callback; }
    void SetOnClientDisconnected(ClientDisconnectCallback callback) { _onClientDisconnected = callback; }
    void SetOnPacketReceived(PacketCallback callback) { _onPacketReceived = callback; }

    // Stats
    uint16_t GetPort() const { return _port; }
    size_t GetMaxClients() const { return _maxClients; }
    uint64_t GetBytesSent() const { return _bytesSent; }
    uint64_t GetBytesReceived() const { return _bytesReceived; }

private:
    void OnClientConnect(ENetPeer* peer);
    void OnClientDisconnect(uint32_t clientId);
    void OnReceive(uint32_t clientId, const uint8_t* data, size_t size);

    uint32_t GenerateClientId();

    ENetHost* _server = nullptr;
    bool _running = false;
    uint16_t _port = 0;
    size_t _maxClients = 512;

    std::unordered_map<uint32_t, ConnectedClient> _clients;
    std::unordered_map<ENetPeer*, uint32_t> _peerToClient;
    uint32_t _nextClientId = 1;

    // Callbacks
    ClientConnectCallback _onClientConnected;
    ClientDisconnectCallback _onClientDisconnected;
    PacketCallback _onPacketReceived;

    // Stats
    uint64_t _bytesSent = 0;
    uint64_t _bytesReceived = 0;
};

} // namespace HogwartsMP::Networking
