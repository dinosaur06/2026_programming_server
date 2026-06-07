#ifndef PHYSICSENGINE_H
#define PHYSICSENGINE_H
#include <cmath>

class PhysicsEngine {
public:
    // 거리 계산을 통해 충돌 여부를 반환
    static bool CheckCollision(float x1, float y1, float x2, float y2);
};
#endif