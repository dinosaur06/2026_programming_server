#include <boost/asio.hpp>
#include "main.h"
#include <iostream>
#include <unordered_map>
#include <cstring>
#include <csignal>
#include "PacketTypes.h"
#include "Monitor.h"
#include "GameManager.h"

using namespace boost::asio;
using udp = ip::udp;

// 전역 io_context 선언 (Ctrl+C 발생 시 안전하게 서버를 종료하기 위함)
boost::asio::io_context* g_ioContext = nullptr;

// Ctrl+C (SIGINT) 시그널 핸들러 함수
void signal_handler(int signal) {
    if (signal == SIGINT && g_ioContext) {
        std::cout << "\n[-] Shutdown signal received. Closing server...\n";
        g_ioContext->stop(); // 서버의 네트워크 루프 중단
    }
}

class FpsServer {
public:
    FpsServer(io_context& ctx, uint16_t port)
        : socket_(ctx, udp::endpoint(udp::v4(), port))
    {
        std::cout << "[Server] Listening on port " << port << "\n";
        do_receive();
    }

    // GameManager 등 외부에서 우회 호출할 수 있도록 public에 배치한 broadcast
    void broadcast(const char* data, std::size_t len, uint32_t exclude_id) {
        for (auto& [id, ep] : clients_) {
            if (id == exclude_id) continue;
            socket_.async_send_to(buffer(data, len), ep,
                [](boost::system::error_code, std::size_t) {});
        }
    }

private:
    udp::socket  socket_;
    udp::endpoint remote_ep_;
    char          recv_buf_[1024];

    // 접속 중인 클라이언트 목록: playerId -> endpoint
    std::unordered_map<uint32_t, udp::endpoint> clients_;

    void do_receive() {
        socket_.async_receive_from(
            buffer(recv_buf_), remote_ep_,
            [this](boost::system::error_code ec, std::size_t bytes) {
                if (!ec && bytes > 0) {
                    // 패킷을 받을 때마다 전역 카운터 증가
                    g_grandTotalPackets++;
                    g_totalTrafficBytes += bytes;

                    handle_packet(bytes);
                }
                do_receive(); // 다음 패킷 대기
            }
        );
    }

    void handle_packet(std::size_t bytes) {
        PacketType type = static_cast<PacketType>(recv_buf_[0]);

        if (type == PacketType::MOVE && bytes >= sizeof(MovePacket)) {
            MovePacket pkt;
            std::memcpy(&pkt, recv_buf_, sizeof(MovePacket));

            // 클라이언트 등록
            clients_[pkt.playerId] = remote_ep_;

            std::cout << "[MOVE] Player " << pkt.playerId
                << " -> (" << pkt.x << ", " << pkt.y << ")\n";

            // [연동] 플레이어가 움직이면 서버 게임 매니저 좌표도 실시간 동기화
            g_gameManager.UpdatePlayerPosition(pkt.playerId, pkt.x, pkt.y);

            // 다른 클라이언트에게 브로드캐스트
            broadcast(recv_buf_, bytes, pkt.playerId);
        }
        else if (type == PacketType::SHOOT && bytes >= sizeof(ShootPacket)) {
            ShootPacket pkt;
            std::memcpy(&pkt, recv_buf_, sizeof(ShootPacket));

            g_pingPacketCount++;

            std::cout << "[SHOOT] Player " << pkt.playerId << " fired a bullet!\n";

            // [연동] 유니티가 쏜 신호를 서버 게임 매니저에 등록
            g_gameManager.CreateBullet(pkt.playerId, pkt.dirX, pkt.dirY);

            // 유니티 클라이언트 전원에게 총알 이펙트를 그리라고 스폰 패킷을 전송
            SpawnBulletPacket spawnPkt;
            spawnPkt.type = PacketType::SPAWN_BULLET;
            spawnPkt.shooterId = pkt.playerId;

            // [오타 수정] 구조체 에러 해결 및 GameManager가 안전하게 동기화해 둔 플레이어의 현재 좌표 추출
            spawnPkt.posX = g_gameManager.GetPlayerX(pkt.playerId);
            spawnPkt.posY = g_gameManager.GetPlayerY(pkt.playerId);
            spawnPkt.dirX = pkt.dirX;
            spawnPkt.dirY = pkt.dirY;

            // 방장(나)을 포함하여 모든 유니티 화면에 총알이 렌더링되도록 exclude_id를 0으로 설정하여 전송
            broadcast(reinterpret_cast<char*>(&spawnPkt), sizeof(SpawnBulletPacket), 0);
        }
        else if (type == PacketType::PING) {
            g_pingPacketCount++;
            std::cout << "[PING] Received from client! Sending PONG...\n";

            char pong = static_cast<char>(PacketType::PING);
            socket_.async_send_to(buffer(&pong, 1), remote_ep_,
                [](boost::system::error_code, std::size_t) {});
        }
    } // 괄호 누락 위치에 정확히 중괄호 추가하여 handle_packet 종료
};

int main() {
    try {
        boost::asio::io_context ctx;
        g_ioContext = &ctx;

        // Ctrl+C 시그널 핸들러 등록
        std::signal(SIGINT, signal_handler);

        // 1. 서버 구동 전 1초 주기 모니터링 스레드 시작
        StartMonitoring();

        // 서버 객체 생성
        FpsServer server(ctx, 9000);

        // GameManager에서 패킷을 쏠 수 있도록 서버 인스턴스 주소를 넘기거나 직접 묶어줍니다.
        g_gameManager.StartGameLoop(&server);

        // 서버 실행 (네트워크 대기)
        ctx.run();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }
    return 0;
}