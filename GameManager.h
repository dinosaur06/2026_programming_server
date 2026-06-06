#pragma once
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include "PacketTypes.h"
#include "player.h"        
#include "PhysicsEngine.h"

class FpsServer; // 전방 선언

class GameManager {
private:
    std::unordered_map<uint32_t, Player> players_;
    PhysicsEngine physics_;
    std::mutex gameMutex_;
    bool isRunning_;

    const float P1_START_X = -5.0f;
    const float P2_START_X = 5.0f;

public:
    GameManager();
    ~GameManager(); // 소멸자 추가

    // 60Hz 독립 물리/규칙 연산 스레드 가동 및 중지
    void StartGameLoop(FpsServer* server);
    void StopGameLoop();

    // 유니티 패킷이 들어왔을 때 main.cpp에서 호출할 연동 함수들
    void UpdatePlayerPosition(uint32_t id, float x, float y);
    void CreateBullet(uint32_t shooterId, float dirX, float dirY);

    // [질문하신 위치] main.cpp에서 안전하게 좌표를 꺼내 쓸 수 있도록 Getter 함수 추가
    float GetPlayerX(uint32_t id) { return players_[id].x; }
    float GetPlayerY(uint32_t id) { return players_[id].y; }

private:
    // 내부에서만 쓰이는 루프 및 라운드 관리 함수들
    void Loop(FpsServer* server);
    void RespawnPlayers(FpsServer* server);
    void BroadcastStatus(FpsServer* server);
};

// 전역 객체 선언을 통해 main.cpp 등에서 쉽게 접근 가능하도록 설정
extern GameManager g_gameManager;