#ifndef GAMEMANAGER_H
#define GAMEMANAGER_H

#include <unordered_map>
#include "player.h" // 기존 player.cpp/h 활용

class GameManager {
public:
    static void ResetRound(std::unordered_map<uint32_t, Player>& players);
    static void CheckWinCondition(uint32_t& p1Score, uint32_t& p2Score);
    static void RespawnPlayer(Player& p);

    // [추가] 총 맞았을 때 처리하는 함수
    static void UpdateScore(uint32_t shooterId, int& targetHp);
};

#endif
