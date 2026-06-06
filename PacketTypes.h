#pragma once
#include <cstdint>
#pragma pack(push, 1)   // 패딩 없이 1바이트 정렬

enum class PacketType : uint8_t {
    MOVE = 0x01,
    SHOOT = 0x02,
    SPAWN_BULLET = 0x03, // 서버 -> 유니티 (총알 생성 브로드캐스트)
    GAME_STATUS = 0x04,   // 서버 -> 유니티 (실시간 HP, 스코어 동기화)    
    PING = 0xFF
};

#pragma pack(push, 1)
// 유니티가 서버로 "나 총 쏜다"라고 보낼 패킷
struct ShootPacket {
    PacketType type;
    uint32_t playerId;
    float dirX;
    float dirY;
};

// 서버가 모든 유니티에게 "누가 어디서 어느 방향으로 총알 생성해라" 할 패킷
struct SpawnBulletPacket {
    PacketType type;
    uint32_t shooterId;
    float posX;
    float posY;
    float dirX;
    float dirY;
};

// 서버가 실시간으로 매 프레임/상황마다 동기화해 줄 전체 게임 데이터 패킷
struct GameStatusPacket {
    PacketType type;
    int p1Hp;
    int p2Hp;
    int p1Score;
    int p2Score;
};

struct MovePacket {
    PacketType type;    // 1 byte
    uint32_t   playerId;// 4 bytes
    float      x, y;// 4 bytes * 3
};  // 총 16 bytes

#pragma pack(pop)
