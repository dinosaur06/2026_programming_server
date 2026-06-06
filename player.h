#pragma once
#include <stdint.h>

class Player {
public:
    uint32_t id;
    float x, y;
    int hp;
    int score;
    bool isConnected;

    Player() : id(0), x(0.0f), y(0.0f), hp(100), score(0), isConnected(false) {}
    void ResetRound(float startX, float startY);
};