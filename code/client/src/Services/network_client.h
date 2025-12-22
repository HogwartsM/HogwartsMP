#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include "../../../shared/networking/network_packet.h"

// Forward declare ENet types
struct _ENetHost;
struct _ENetPeer;
typedef _ENetHost ENetHost;
typedef _ENetPeer ENetPeer;

namespace HogwartsMP::Networking {

using PacketCallback = std::function<void(PacketType, const uint8_t*, size_t)>;

class NetworkClient {
public:
    NetworkClient();
    ~NetworkClient();

    // Connection management
    bool Connect(const std::string& hostname, uint16_t port, uint32_t timeout = 5000);
    void Disconnect();
    bool IsConnected() const { return _connected; }

    // Update (call every frame)
    void Update();

    // Send packet
    bool SendPacket(const NetworkPacket& packet, bool reliable = true);
    bool SendRaw(const std::vector<uint8_t>& data, bool reliable = true);

    // Callbacks
    void SetOnConnected(std::function<void()> callback) { _onConnected = callback; }
    void SetOnDisconnected(std::function<void()> callback) { _onDisconnected = callback; }
    void SetOnPacketReceived(PacketCallback callback) { _onPacketReceived = callback; }

    // Stats
    uint32_t GetPing() const;
    uint64_t GetBytesSent() const;
    uint64_t GetBytesReceived() const;

private:
    void OnConnect();
    void OnDisconnect();
    void OnReceive(const uint8_t* data, size_t size);

    ENetHost* _client = nullptr;
    ENetPeer* _peer = nullptr;
    bool _connected = false;

    // Callbacks
    std::function<void()> _onConnected;
    std::function<void()> _onDisconnected;
    PacketCallback _onPacketReceived;

    // Stats
    uint64_t _bytesSent = 0;
    uint64_t _bytesReceived = 0;
};

} // namespace HogwartsMP::Networking
