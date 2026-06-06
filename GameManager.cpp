#include "GameManager.h"
#include "PacketTypes.h"
#include "main.h"
#include <thread>
#include <chrono>
#include <cmath>
#include <iostream>
#include <boost/asio.hpp>

// 전역 싱글톤 인스턴스 정의
GameManager g_gameManager;

GameManager::GameManager() : isRunning_(false) {
    // 1번, 2번 플레이어의 초기 ID와 기본 접속 상태 세팅
    players_[1].id = 1;
    players_[1].x = P1_START_X;
    players_[1].y = 0.0f;
    players_[1].hp = 100;
    players_[1].score = 0;
    players_[1].isConnected = false;

    players_[2].id = 2;
    players_[2].x = P2_START_X;
    players_[2].y = 0.0f;
    players_[2].hp = 100;
    players_[2].score = 0;
    players_[2].isConnected = false;
}

GameManager::~GameManager() {
    StopGameLoop();
}

// 유니티에서 이동 패킷(MOVE)이 들어오면 서버 기록실의 플레이어 좌표를 실시간 갱신
void GameManager::UpdatePlayerPosition(uint32_t id, float x, float y) {
    if (id < 1 || id > 2) return;
    std::lock_guard<std::mutex> lock(gameMutex_);
    players_[id].x = x;
    players_[id].y = y;
    players_[id].isConnected = true;
}

// 유니티에서 발사 패킷(SHOOT)이 들어오면 PhysicsEngine에 총알 등록 요청
void GameManager::CreateBullet(uint32_t shooterId, float dirX, float dirY) {
    std::lock_guard<std::mutex> lock(gameMutex_);
    if (players_[shooterId].hp <= 0) return; // 죽은 플레이어는 발사 불가

    // 이미 가지고 계신 physics_ 객체의 AddBullet을 호출하여 안전하게 위임합니다.
    physics_.AddBullet(
        shooterId,
        players_[shooterId].x,
        players_[shooterId].y,
        dirX,
        dirY
    );
}

// 1초에 60번 독립적으로 작동할 게임 연산 스레드 가동
void GameManager::StartGameLoop(FpsServer* server) {
    if (isRunning_) return;
    isRunning_ = true;
    std::thread([this, server]() { this->Loop(server); }).detach();
    std::cout << "[GameManager] Physics & Rule Engine Loop Started (60Hz).\n";
}

void GameManager::StopGameLoop() {
    isRunning_ = false;
}

// 핵심 물리 엔진 업데이트 및 규칙 판정 루프
void GameManager::Loop(FpsServer* server) {
    const int FPS = 60;
    const auto FRAME_TIME = std::chrono::milliseconds(1000 / FPS);
    const float DELTA_TIME = 1.0f / FPS;

    while (isRunning_) {
        auto startTime = std::chrono::steady_clock::now();

        {
            std::lock_guard<std::mutex> lock(gameMutex_);

            // 1. 이미 작성해두신 PhysicsEngine의 함수를 이용해 총알들을 이동시킵니다.
            physics_.UpdatePhysics(DELTA_TIME);

            // 2. 플레이어들과 총알 간의 2D 충돌(피격)을 검사합니다.
            uint32_t hitPlayerId = physics_.CheckCollisions(players_);

            if (hitPlayerId > 0) {
                // 피격당한 사람이 생겼다면 콘솔에 로그 출력
                std::cout << "[HIT] Player " << hitPlayerId << " got shot! Remaining HP: " << players_[hitPlayerId].hp << "\n";

                // 라운드 종료 조건 체크 (피격된 플레이어의 체력이 0 이하가 됨)
                if (players_[hitPlayerId].hp <= 0) {
                    uint32_t winnerId = (hitPlayerId == 1) ? 2 : 1;
                    players_[winnerId].score++;
                    std::cout << "[ROUND OVER] Winner is Player " << winnerId << "!\n";

                    // 라운드 리셋 진행
                    RespawnPlayers(server);
                }
                else {
                    // 체력만 깎인 상태라면 실시간 HP 상황을 유니티들에게 즉시 브로드캐스트
                    BroadcastStatus(server);
                }
            }
        }

        // 60Hz 정밀 대기
        auto endTime = std::chrono::steady_clock::now();
        auto elapsedTime = endTime - startTime;
        if (elapsedTime < FRAME_TIME) {
            std::this_thread::sleep_for(FRAME_TIME - elapsedTime);
        }
    }
}

// 라운드가 끝났을 때 모든 오브젝트 원위치 및 초기화
void GameManager::RespawnPlayers(FpsServer* server) {
    // Player.cpp에 이미 만들어두신 ResetRound 함수를 완벽하게 활용합니다
    players_[1].ResetRound(P1_START_X, 0.0f);
    players_[2].ResetRound(P2_START_X, 0.0f);

    // 1단계에서 PhysicsEngine.cpp에 추가한 ClearBullets를 호출하여 필드의 총알을 청소합니다.
    physics_.ClearBullets();

    // 리셋된 조화로운 상태를 유니티 클라이언트 전원에게 전송
    BroadcastStatus(server);
}

// 현재 게임 세계관의 상태 패킷(HP, Score)을 가공하여 모든 클라이언트에게 전송
void GameManager::BroadcastStatus(FpsServer* server) {
    GameStatusPacket pkt;
    pkt.type = PacketType::GAME_STATUS;
    pkt.p1Hp = players_[1].hp;
    pkt.p2Hp = players_[2].hp;
    pkt.p1Score = players_[1].score;
    pkt.p2Score = players_[2].score;

    // FpsServer의 전역 브로드캐스트 통로를 통해 패킷 전송
    server->broadcast(reinterpret_cast<char*>(&pkt), sizeof(GameStatusPacket), 0);
}