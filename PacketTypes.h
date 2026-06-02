#pragma once
#include <cstdint>
#pragma pack(push, 1)   // 패딩 없이 1바이트 정렬

enum class PacketType : uint8_t {
    MOVE = 0x01,
    SHOOT = 0x02,
    HIT = 0x03,
    PING = 0xFF
};

struct MovePacket {
    PacketType type;    // 1 byte
    uint32_t   playerId;// 4 bytes
    float      x, y;// 4 bytes * 3
};  // 총 16 bytes

#pragma pack(pop)
