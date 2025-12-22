#include "network_client.h"
#include "../../../shared/logging/logger.h"
#include <enet/enet.h>

namespace HogwartsMP::Networking {

NetworkClient::NetworkClient() {
    // Initialize ENet
    if (enet_initialize() != 0) {
        Logging::Logger::Error("Failed to initialize ENet");
    } else {
        Logging::Logger::Info("ENet initialized successfully");
    }
}

NetworkClient::~NetworkClient() {
    Disconnect();

    if (_client) {
        enet_host_destroy(_client);
        _client = nullptr;
    }

    enet_deinitialize();
}

bool NetworkClient::Connect(const std::string& hostname, uint16_t port, uint32_t timeout) {
    if (_connected) {
        Logging::Logger::Warning("Already connected to server");
        return false;
    }

    // Create client
    _client = enet_host_create(
        nullptr,  // create a client host
        1,        // only allow 1 outgoing connection
        32,       // allow up to 32 channels
        0,        // assume any amount of incoming bandwidth
        0         // assume any amount of outgoing bandwidth
    );

    if (_client == nullptr) {
        Logging::Logger::Error("Failed to create ENet client host");
        return false;
    }

    // Set server address
    ENetAddress address;
    enet_address_set_host(&address, hostname.c_str());
    address.port = port;

    // Initiate connection
    _peer = enet_host_connect(_client, &address, 32, 0);

    if (_peer == nullptr) {
        Logging::Logger::Error("No available peers for initiating an ENet connection");
        enet_host_destroy(_client);
        _client = nullptr;
        return false;
    }

    Logging::Logger::InfoF("Connecting to %s:%d...", hostname.c_str(), port);

    // Wait for connection to succeed
    ENetEvent event;
    // Increase timeout to ensure connection
    if (enet_host_service(_client, &event, timeout) > 0 &&
        event.type == ENET_EVENT_TYPE_CONNECT) {
        OnConnect();
        return true;
    } else {
        Logging::Logger::Error("Connection to server failed (timeout)");
        enet_peer_reset(_peer);
        _peer = nullptr;
        // Don't destroy client here, we might want to retry later
        // But for now, user requested strict blocking.
        // Actually, ClientInstance::Init fails if we return false, so destroying is fine/correct for this flow.
        enet_host_destroy(_client);
        _client = nullptr;
        return false;
    }
}

void NetworkClient::Disconnect() {
    if (!_connected || !_peer) {
        return;
    }

    Logging::Logger::Info("Disconnecting from server...");

    enet_peer_disconnect(_peer, 0);

    // Wait for disconnect event
    ENetEvent event;
    while (enet_host_service(_client, &event, 3000) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_RECEIVE:
                enet_packet_destroy(event.packet);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                OnDisconnect();
                return;
        }
    }

    // Force disconnect
    enet_peer_reset(_peer);
    OnDisconnect();
}

void NetworkClient::Update() {
    if (!_client) {
        return;
    }

    ENetEvent event;
    while (enet_host_service(_client, &event, 0) > 0) {
        switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                OnConnect();
                break;

            case ENET_EVENT_TYPE_RECEIVE:
                OnReceive(event.packet->data, event.packet->dataLength);
                _bytesReceived += event.packet->dataLength;
                enet_packet_destroy(event.packet);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                OnDisconnect();
                break;

            default:
                break;
        }
    }
}

bool NetworkClient::SendPacket(const NetworkPacket& packet, bool reliable) {
    auto data = packet.Serialize();
    return SendRaw(data, reliable);
}

bool NetworkClient::SendRaw(const std::vector<uint8_t>& data, bool reliable) {
    if (!_connected || !_peer) {
        Logging::Logger::Warning("Cannot send packet: not connected");
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

    if (enet_peer_send(_peer, 0, packet) < 0) {
        Logging::Logger::Error("Failed to send packet");
        return false;
    }

    _bytesSent += data.size();
    return true;
}

uint32_t NetworkClient::GetPing() const {
    if (_peer) {
        return _peer->roundTripTime;
    }
    return 0;
}

uint64_t NetworkClient::GetBytesSent() const {
    return _bytesSent;
}

uint64_t NetworkClient::GetBytesReceived() const {
    return _bytesReceived;
}

void NetworkClient::OnConnect() {
    _connected = true;
    Logging::Logger::Info("Connected to server successfully");

    if (_onConnected) {
        _onConnected();
    }
}

void NetworkClient::OnDisconnect() {
    _connected = false;
    _peer = nullptr;
    Logging::Logger::Info("Disconnected from server");

    if (_onDisconnected) {
        _onDisconnected();
    }
}

void NetworkClient::OnReceive(const uint8_t* data, size_t size) {
    if (size < 1) {
        return;
    }

    // First byte is packet type
    PacketType type = static_cast<PacketType>(data[0]);

    if (_onPacketReceived) {
        _onPacketReceived(type, data + 1, size - 1);
    }
}

} // namespace HogwartsMP::Networking
