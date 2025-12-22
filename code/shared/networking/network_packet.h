#pragma once

#include <vector>
#include <cstdint>
#include <string>

namespace HogwartsMP::Networking {

// Packet types (identifiers)
enum class PacketType : uint8_t {
    // Connection
    Connect = 0,
    Disconnect = 1,

    // Game sync
    PlayerJoin = 10,
    PlayerLeave = 11,
    PlayerUpdate = 12,

    // Events
    Event = 20,

    // Resources
    ResourceList = 30,
    ResourceRequest = 31,
    ResourceManifest = 32,
    ResourceFile = 33,
    ResourceComplete = 34,

    // Chat
    ChatMessage = 40,

    // Coordinates
    RequestPlayerCoordinates = 41,

    // RPC
    RPC = 50,

    // Custom (for future use)
    Custom = 255
};

class NetworkPacket {
public:
    NetworkPacket() = default;
    NetworkPacket(PacketType type) : _type(type) {}
    virtual ~NetworkPacket() = default;

    // Serialize packet to bytes
    virtual std::vector<uint8_t> Serialize() const = 0;

    // Deserialize packet from bytes
    virtual bool Deserialize(const uint8_t* data, size_t size) = 0;

    PacketType GetType() const { return _type; }
    void SetType(PacketType type) { _type = type; }

protected:
    PacketType _type = PacketType::Custom;

    // Helper functions for serialization
    static void WriteUInt8(std::vector<uint8_t>& buffer, uint8_t value);
    static void WriteUInt16(std::vector<uint8_t>& buffer, uint16_t value);
    static void WriteUInt32(std::vector<uint8_t>& buffer, uint32_t value);
    static void WriteUInt64(std::vector<uint8_t>& buffer, uint64_t value);
    static void WriteFloat(std::vector<uint8_t>& buffer, float value);
    static void WriteString(std::vector<uint8_t>& buffer, const std::string& value);
    static void WriteBytes(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& value);

    static uint8_t ReadUInt8(const uint8_t** data);
    static uint16_t ReadUInt16(const uint8_t** data);
    static uint32_t ReadUInt32(const uint8_t** data);
    static uint64_t ReadUInt64(const uint8_t** data);
    static float ReadFloat(const uint8_t** data);
    static std::string ReadString(const uint8_t** data);
    static std::vector<uint8_t> ReadBytes(const uint8_t** data);
};

} // namespace HogwartsMP::Networking
