#include "GameManager.h"
#include <iostream>

// 라운드 종료 시 플레이어 체력 리셋
void GameManager::ResetRound(std::unordered_map<uint32_t, Player>& players) {
    for (auto& [id, p] : players) {
        p.hp = 100;
        // 필요 시 p.x, p.y를 시작 좌표로 강제 이동시키는 로직 추가
    }
    std::cout << "[GameManager] 라운드가 초기화되었습니다." << std::endl;
}

// 점수 체크 (2점 먼저 내면 승리)
void GameManager::CheckWinCondition(uint32_t& p1Score, uint32_t& p2Score) {
    if (p1Score >= 2) {
        std::cout << "[GameManager] Player 1 최종 승리!" << std::endl;
        p1Score = 0; p2Score = 0; // 게임 전체 초기화
    }
    else if (p2Score >= 2) {
        std::cout << "[GameManager] Player 2 최종 승리!" << std::endl;
        p1Score = 0; p2Score = 0;
    }
}

void GameManager::RespawnPlayer(Player& p) {
    p.hp = 100;
    // p.x, p.y = StartPos; 등 초기 위치 지정
}

void GameManager::UpdateScore(uint32_t shooterId, int& targetHp) {
    targetHp -= 20; // 서버가 결정하는 데미지
    std::cout << "[GameManager] 피격! 남은 체력: " << targetHp << std::endl;

    if (targetHp <= 0) {
        std::cout << "[GameManager] 플레이어 " << shooterId << "가 승점 획득!" << std::endl;
        targetHp = 100; // 리스폰
    }
}