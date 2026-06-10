#include "PhysicsEngine.h"

bool PhysicsEngine::CheckCollision(float x1, float y1, float x2, float y2) {
    float dx = x1 - x2;
    float dy = y1 - y2;
    float distance = std::sqrt(dx * dx + dy * dy);
    return distance < 1.0f; // 1.0 : 캐릭터의 히트박스 반경
}
