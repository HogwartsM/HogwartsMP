#include "network_client.h"
#include "../../../shared/logging/logger.h"
#include <enet/enet.h>
#include <iostream>

namespace HogwartsMP::Networking {

    NetworkClient::NetworkClient() {
        if (enet_initialize() != 0) {
            Logging::Logger::Error("Failed to initialize ENet in Client.");
        } else {
            Logging::Logger::Info("ENet initialized successfully in Client.");
        }
    }

    NetworkClient::~NetworkClient() {
        Disconnect();
        if (_host) {
            enet_host_destroy(_host);
            _host = nullptr;
        }
        enet_deinitialize();
    }

    bool NetworkClient::Connect(const std::string& hostname, uint16_t port, uint32_t timeoutMs) {
        if (_connected) {
            Logging::Logger::Warning("Client already connected.");
            return false;
        }

        // Create client host
        // We only need 1 outgoing connection, but let's allow 2 to be safe or flexible
        _host = enet_host_create(nullptr, 1, 2, 0, 0);
        if (_host == nullptr) {
            Logging::Logger::Error("Failed to create ENet client host.");
            return false;
        }

        ENetAddress address;
        enet_address_set_host(&address, hostname.c_str());
        address.port = port;

        Logging::Logger::InfoF("Attempting to connect to %s:%d...", hostname.c_str(), port);

        // Initiate connection
        _peer = enet_host_connect(_host, &address, 2, 0);
        if (_peer == nullptr) {
            Logging::Logger::Error("No available peers for initiating an ENet connection.");
            enet_host_destroy(_host);
            _host = nullptr;
            return false;
        }

        // Wait for connection to succeed
        ENetEvent event;
        // enet_host_service will block up to timeoutMs
        if (enet_host_service(_host, &event, timeoutMs) > 0 &&
            event.type == ENET_EVENT_TYPE_CONNECT) {
            
            HandleConnect();
            return true;
        } else {
            Logging::Logger::Error("Connection failed or timed out.");
            enet_peer_reset(_peer);
            _peer = nullptr;
            enet_host_destroy(_host);
            _host = nullptr;
            return false;
        }
    }

    void NetworkClient::Disconnect() {
        if (!_connected || !_peer) return;

        Logging::Logger::Info("Disconnecting...");
        enet_peer_disconnect(_peer, 0);

        // Allow disconnect packet to be sent
        ENetEvent event;
        bool disconnected = false;
        
        // Wait up to 3 seconds for disconnect acknowledgement
        // We need to loop because we might receive other packets while waiting
        uint32_t startTime = enet_time_get();
        while (enet_time_get() - startTime < 3000) {
             if (enet_host_service(_host, &event, 100) > 0) {
                switch (event.type) {
                    case ENET_EVENT_TYPE_RECEIVE:
                        enet_packet_destroy(event.packet);
                        break;
                    case ENET_EVENT_TYPE_DISCONNECT:
                        disconnected = true;
                        goto done;
                }
             }
        }
        
        done:
        if (!disconnected) {
            // Force reset if not graceful
            enet_peer_reset(_peer);
        }
        
        HandleDisconnect();
    }

    void NetworkClient::Update() {
        if (!_host) return;

        ENetEvent event;
        while (enet_host_service(_host, &event, 0) > 0) {
            switch (event.type) {
                case ENET_EVENT_TYPE_CONNECT:
                    // Should be handled in Connect(), but if it happens later (reconnect?), handle it.
                    // Actually, Connect() handles the initial one.
                    // If we get another CONNECT, it's weird for a client host with 1 peer.
                    HandleConnect();
                    break;
                case ENET_EVENT_TYPE_RECEIVE:
                    HandleReceive(event.packet->data, event.packet->dataLength);
                    enet_packet_destroy(event.packet);
                    break;
                case ENET_EVENT_TYPE_DISCONNECT:
                    HandleDisconnect();
                    break;
                default:
                    break;
            }
        }
    }

    bool NetworkClient::SendPacket(const NetworkPacket& packet, bool reliable) {
        std::vector<uint8_t> data = packet.Serialize();
        return SendRaw(data, reliable);
    }

    bool NetworkClient::SendRaw(const std::vector<uint8_t>& data, bool reliable) {
        if (!_connected || !_peer) return false;

        ENetPacket* packet = enet_packet_create(data.data(), data.size(), reliable ? ENET_PACKET_FLAG_RELIABLE : 0);
        if (enet_peer_send(_peer, 0, packet) < 0) {
            Logging::Logger::Error("Failed to send packet from client.");
            return false;
        }

        _bytesSent += data.size();
        return true;
    }

    uint32_t NetworkClient::GetPing() const {
        return _peer ? _peer->roundTripTime : 0;
    }

    void NetworkClient::HandleConnect() {
        _connected = true;
        Logging::Logger::Info("Client connected to server!");
        if (_onConnected) {
            _onConnected();
        }
    }

    void NetworkClient::HandleDisconnect() {
        _connected = false;
        _peer = nullptr;
        Logging::Logger::Info("Client disconnected.");
        if (_onDisconnected) {
            _onDisconnected();
        }
    }

    void NetworkClient::HandleReceive(const uint8_t* data, size_t size) {
        _bytesReceived += size;
        if (size < 1) return;

        PacketType type = static_cast<PacketType>(data[0]);
        if (_onPacketReceived) {
            _onPacketReceived(type, data, size);
        }
    }

} // namespace HogwartsMP::Networking
