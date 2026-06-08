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
                    // 데이터를 안전하게 복사
                    std::vector<char> packetData(recv_buf_, recv_buf_ + bytes);

                    g_packetCount++;
                    // 복사본을 넘겨줌
                    handle_packet(packetData);
                }
                do_receive();
            }
        );
    }


    void handle_packet(const std::vector<char>& data) {
        size_t bytes = data.size();
        if (bytes < 1) return;

        // 데이터 접근은 data.data()를 사용
        PacketType type = (PacketType)data[0];

        if (type == PacketType::MOVE) {
            MovePacket movePkt;
            std::memcpy(&movePkt, data.data(), sizeof(MovePacket));

            uint32_t playerId = movePkt.playerId; // 패킷에서 ID 추출

            // 1. 플레이어가 맵에 없으면 등록
            if (clients_.find(playerId) == clients_.end()) {
                // 임시로 빈 Player 생성 후 초기화
                Player newPlayer;
                InitPlayer(newPlayer, playerId, remote_ep_);
                clients_[playerId] = newPlayer;
            }

            // 2. 위치 갱신
            clients_[playerId].x = movePkt.x;
            clients_[playerId].y = movePkt.y;

            // 3. 브로드캐스트
            broadcast(data.data(), bytes, playerId);
        }
        else if (type == PacketType::SHOOT) {
            ShootPacket pkt;
            std::memcpy(&pkt, data.data(), sizeof(ShootPacket));

            uint32_t shooterId = pkt.playerId;
            // 타겟 ID 선정 로직: 실제 게임에서는 현재 접속된 클라이언트 리스트에서 
            // 쏘는 사람(shooterId)과 다른 플레이어를 찾아야 합니다.
            for (auto& [id, player] : clients_) {
                if (id != shooterId) {
                    uint32_t targetId = id;
                    // 충돌 판정 및 점수 로직
                    if (PhysicsEngine::CheckCollision(clients_[shooterId].x, clients_[shooterId].y,
                        clients_[targetId].x, clients_[targetId].y)) {

                        GameManager::UpdateScore(shooterId, clients_[targetId].hp);

                        HitPacket hitPkt;
                        hitPkt.type = PacketType::HIT; // 타입 명시
                        hitPkt.playerId = targetId;
                        hitPkt.currentHp = clients_[targetId].hp;

                        broadcast(reinterpret_cast<char*>(&hitPkt), sizeof(HitPacket), 0);
                    }
                }
            }
        }
    }


    void broadcast(const char* data, std::size_t len, uint32_t exclude_id) {
        for (auto& [id, player] : clients_) {
            if (id == exclude_id) continue;

            std::cout << "[DEBUG] " << id << "번 플레이어에게 전송 중! IP: " << player.ep.address() << std::endl;
            // endpoint 정보가 player 구조체에 있어야 함
            socket_.async_send_to(buffer(data, len), player.ep, [](auto, auto) {});
        }
    }
};

int main() {
    std::cout << "MovePacket size: " << sizeof(MovePacket) << " bytes" << std::endl;
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