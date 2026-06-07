#include <boost/asio.hpp>
#include <iostream>
#include <unordered_map>
#include <cstring>
#include <csignal> 
#include "PacketTypes.h"
#include "Monitor.h"
#include "GameManager.h"
#include "PhysicsEngine.h"
#include "player.h"

using namespace boost::asio;
using udp = ip::udp;

boost::asio::io_context* g_ioContext = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT && g_ioContext) {
        StopMonitoring();
        g_ioContext->stop();
    }
}

class FpsServer {
public:
    FpsServer(io_context& ctx, uint16_t port)
        : socket_(ctx, udp::endpoint(udp::v4(), port)) {
        std::cout << "[Server] Listening on port " << port << "\n";
        do_receive();
    }

private:
    udp::socket socket_;
    udp::endpoint remote_ep_;
    char recv_buf_[1024];
    std::unordered_map<uint32_t, Player> clients_;

    uint32_t p1Score = 0;
    uint32_t p2Score = 0;

    void do_receive() {
        socket_.async_receive_from(
            buffer(recv_buf_), remote_ep_,
            [this](boost::system::error_code ec, std::size_t bytes) {
                if (!ec && bytes > 0) {
                    handle_packet(bytes);
                }
                do_receive();
            }
        );
    }

    void handle_packet(std::size_t bytes) {
        PacketType type = static_cast<PacketType>(recv_buf_[0]);

        // [추가] 패킷을 보낸 플레이어가 서버에 등록되어 있는지 확인하고, 없으면 등록!
    // MOVE 패킷 등을 통해 playerId를 알 수 있다고 가정합니다.
        if (type == PacketType::MOVE) {
            MovePacket movePkt;
            std::memcpy(&movePkt, recv_buf_, sizeof(MovePacket));

            uint32_t playerId = movePkt.playerId;
            if (clients_.find(playerId) == clients_.end()) {
                Player newPlayer;
                InitPlayer(newPlayer, playerId, remote_ep_); // 초기화 함수 호출!
                clients_[playerId] = newPlayer;
                std::cout << "[Server] Player " << playerId << " initialized!\n";
            }
            // 그 후 플레이어 위치 갱신
            clients_[playerId].x = movePkt.x;
            clients_[playerId].y = movePkt.y;

            broadcast(recv_buf_, bytes, playerId);
        }

        if (type == PacketType::SHOOT) {
            ShootPacket pkt;
            std::memcpy(&pkt, recv_buf_, sizeof(ShootPacket));

            uint32_t shooterId = pkt.playerId;
            // targetId를 찾는 로직: 쏘는 사람 외의 플레이어를 타겟으로 설정
            uint32_t targetId = (shooterId == 1) ? 2 : 1;

            if (clients_.count(targetId)) {
                // 1. 충돌 판정
                if (PhysicsEngine::CheckCollision(clients_[shooterId].x, clients_[shooterId].y,
                    clients_[targetId].x, clients_[targetId].y)) {

                    // 2. 체력 깎기
                    GameManager::UpdateScore(shooterId, clients_[targetId].hp);

                    // 3. 클라이언트들에게 피격 결과 전달 (HitPacket 필요)
                    HitPacket hitPkt;
                    hitPkt.playerId = targetId;
                    hitPkt.currentHp = clients_[targetId].hp;
                    broadcast(reinterpret_cast<char*>(&hitPkt), sizeof(HitPacket), 0);

                    // 4. 점수 체크
                    if (clients_[targetId].hp >= 100) { // 리스폰(100) 되었을 때 점수 처리
                        if (shooterId == 1) p1Score++; else p2Score++;
                        GameManager::CheckWinCondition(p1Score, p2Score);
                    }
                }
            }
        }
    }

    void broadcast(const char* data, std::size_t len, uint32_t exclude_id) {
        for (auto& [id, player] : clients_) {
            if (id == exclude_id) continue;
            // endpoint 정보가 player 구조체에 있어야 함
            socket_.async_send_to(buffer(data, len), player.ep, [](auto, auto) {});
        }
    }
};

int main() {
    StartMonitoring();
    try {
        boost::asio::io_context ioContext;
        g_ioContext = &ioContext;

        // SIGINT(Ctrl+C) 신호 처리 등록
        std::signal(SIGINT, signal_handler);

        // 서버 포트 9000번으로 시작
        FpsServer server(ioContext, 9000);

        // io_context 실행 (서버 루프 시작)
        ioContext.run();
    }
    catch (std::exception& e) {
        std::cerr << "[Error] " << e.what() << std::endl;
    }

    return 0;
}