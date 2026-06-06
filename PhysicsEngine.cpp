#include "PhysicsEngine.h"
#include <cmath>

void PhysicsEngine::AddBullet(uint32_t shooterId, float startX, float startY, float dirX, float dirY) {
    bullets_.push_back({ shooterId, startX, startY, dirX, dirY, 5.0f });
}

void PhysicsEngine::UpdatePhysics(float deltaTime) {
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        it->posX += it->dirX * BULLET_SPEED * deltaTime;
        it->posY += it->dirY * BULLET_SPEED * deltaTime;
        it->lifetime -= deltaTime;

        if (it->lifetime <= 0.0f) {
            it = bullets_.erase(it);
        }
        else {
            ++it;
        }
    }
}

uint32_t PhysicsEngine::CheckCollisions(std::unordered_map<uint32_t, Player>& players) {
    for (auto it = bullets_.begin(); it != bullets_.end();) {
        bool bulletDestroyed = false;

        for (auto& [id, player] : players) {
            if (!player.isConnected || player.hp <= 0) continue;
            if (it->shooterId == id) continue; // 피아식별 (내가 쏜 건 패스)

            float dx = it->posX - player.x;
            float dy = it->posY - player.y;
            float distance = std::sqrt(dx * dx + dy * dy);

            if (distance < HIT_RADIUS) {
                player.hp -= 10; // HP 차감
                bulletDestroyed = true;

                it = bullets_.erase(it); // 총알 삭제
                return player.id;        // 피격된 플레이어 ID 반환
            }
        }

        if (!bulletDestroyed) {
            ++it;
        }
    }
    return 0; // 아무도 안 맞음
}

void PhysicsEngine::ClearBullets() {
    bullets_.clear();
}