#pragma once
#include <vector>
#include <unordered_map>
#include "Player.h"

struct ServerBullet {
    uint32_t shooterId;
    float posX, posY;
    float dirX, dirY;
    float lifetime;
};

class PhysicsEngine {
private:
    std::vector<ServerBullet> bullets_;
    const float BULLET_SPEED = 22.0f;
    const float HIT_RADIUS = 0.6f;

public:
    void AddBullet(uint32_t shooterId, float startX, float startY, float dirX, float dirY);
    void UpdatePhysics(float deltaTime);

    // 피격이 발생했는지 검사하고, 맞은 플레이어의 ID를 반환 (없으면 0 반환)
    uint32_t CheckCollisions(std::unordered_map<uint32_t, Player>& players);
    void ClearBullets();
};