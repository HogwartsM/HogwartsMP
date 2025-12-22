#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include "../../../shared/networking/network_packet.h"

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

        // Prevent copying
        NetworkClient(const NetworkClient&) = delete;
        NetworkClient& operator=(const NetworkClient&) = delete;

        // Connection
        bool Connect(const std::string& hostname, uint16_t port, uint32_t timeoutMs = 5000);
        void Disconnect();
        bool IsConnected() const { return _connected; }

        // Update loop
        void Update();

        // Sending
        bool SendPacket(const NetworkPacket& packet, bool reliable = true);
        bool SendRaw(const std::vector<uint8_t>& data, bool reliable = true);

        // Callbacks
        void SetOnConnected(std::function<void()> callback) { _onConnected = callback; }
        void SetOnDisconnected(std::function<void()> callback) { _onDisconnected = callback; }
        void SetOnPacketReceived(PacketCallback callback) { _onPacketReceived = callback; }

        // Stats
        uint32_t GetPing() const;
        uint64_t GetBytesSent() const { return _bytesSent; }
        uint64_t GetBytesReceived() const { return _bytesReceived; }

    private:
        void HandleConnect();
        void HandleDisconnect();
        void HandleReceive(const uint8_t* data, size_t size);

        ENetHost* _host = nullptr;
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
