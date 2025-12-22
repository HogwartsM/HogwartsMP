#include "network_packet.h"
#include <cstring>

namespace HogwartsMP::Networking {

void NetworkPacket::WriteUInt8(std::vector<uint8_t>& buffer, uint8_t value) {
    buffer.push_back(value);
}

void NetworkPacket::WriteUInt16(std::vector<uint8_t>& buffer, uint16_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void NetworkPacket::WriteUInt32(std::vector<uint8_t>& buffer, uint32_t value) {
    buffer.push_back(static_cast<uint8_t>(value & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    buffer.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
}

void NetworkPacket::WriteUInt64(std::vector<uint8_t>& buffer, uint64_t value) {
    for (int i = 0; i < 8; i++) {
        buffer.push_back(static_cast<uint8_t>((value >> (i * 8)) & 0xFF));
    }
}

void NetworkPacket::WriteFloat(std::vector<uint8_t>& buffer, float value) {
    uint32_t intValue;
    std::memcpy(&intValue, &value, sizeof(float));
    WriteUInt32(buffer, intValue);
}

void NetworkPacket::WriteString(std::vector<uint8_t>& buffer, const std::string& value) {
    WriteUInt32(buffer, static_cast<uint32_t>(value.size()));
    buffer.insert(buffer.end(), value.begin(), value.end());
}

void NetworkPacket::WriteBytes(std::vector<uint8_t>& buffer, const std::vector<uint8_t>& value) {
    WriteUInt32(buffer, static_cast<uint32_t>(value.size()));
    buffer.insert(buffer.end(), value.begin(), value.end());
}

uint8_t NetworkPacket::ReadUInt8(const uint8_t** data) {
    uint8_t value = **data;
    (*data)++;
    return value;
}

uint16_t NetworkPacket::ReadUInt16(const uint8_t** data) {
    uint16_t value = static_cast<uint16_t>((*data)[0]) |
                     (static_cast<uint16_t>((*data)[1]) << 8);
    (*data) += 2;
    return value;
}

uint32_t NetworkPacket::ReadUInt32(const uint8_t** data) {
    uint32_t value = static_cast<uint32_t>((*data)[0]) |
                     (static_cast<uint32_t>((*data)[1]) << 8) |
                     (static_cast<uint32_t>((*data)[2]) << 16) |
                     (static_cast<uint32_t>((*data)[3]) << 24);
    (*data) += 4;
    return value;
}

uint64_t NetworkPacket::ReadUInt64(const uint8_t** data) {
    uint64_t value = 0;
    for (int i = 0; i < 8; i++) {
        value |= static_cast<uint64_t>((*data)[i]) << (i * 8);
    }
    (*data) += 8;
    return value;
}

float NetworkPacket::ReadFloat(const uint8_t** data) {
    uint32_t intValue = ReadUInt32(data);
    float value;
    std::memcpy(&value, &intValue, sizeof(float));
    return value;
}

std::string NetworkPacket::ReadString(const uint8_t** data) {
    uint32_t length = ReadUInt32(data);
    std::string value(reinterpret_cast<const char*>(*data), length);
    (*data) += length;
    return value;
}

std::vector<uint8_t> NetworkPacket::ReadBytes(const uint8_t** data) {
    uint32_t length = ReadUInt32(data);
    std::vector<uint8_t> value(*data, *data + length);
    (*data) += length;
    return value;
}

} // namespace HogwartsMP::Networking
