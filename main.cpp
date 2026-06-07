#include <boost/asio.hpp>
#include <iostream>
#include <unordered_map>
#include <cstring>
#include <csignal> 
#include "PacketTypes.h"
#include "Monitor.h"

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
                    // 패킷을 받을 때마다 모니터링 카운터 증가 (Monitor.h와 연결)
                    g_packetCount++;
                    g_totalBytes += bytes;

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

            // 다른 클라이언트에게 브로드캐스트
            broadcast(recv_buf_, bytes, pkt.playerId);
        }
        else if (type == PacketType::PING) {
            // Pong 응답
            std::cout << "[PING] Received from client! Sending PONG...\n";
            char pong = static_cast<char>(PacketType::PING);
            socket_.async_send_to(buffer(&pong, 1), remote_ep_,
                [](boost::system::error_code, std::size_t) {});
        }
    }

    void broadcast(const char* data, std::size_t len, uint32_t exclude_id) {
        for (auto& [id, ep] : clients_) {
            if (id == exclude_id) continue;
            socket_.async_send_to(buffer(data, len), ep,
                [](boost::system::service_ptr, std::size_t) {});
        }
    }
};

int main() {
    try {
        boost::asio::io_context ctx;
        g_ioContext = &ctx;

        // Ctrl+C 시그널 핸들러 등록
        std::signal(SIGINT, signal_handler);

        // 서버 구동 전 1초 주기 모니터링 스레드 시작!
        StartMonitoring();

        // 서버 객체 생성
        FpsServer server(ctx, 9000);

        // 서버 실행 (네트워크 대기)
        ctx.run();

        // ctx.run()이 종료되면(Ctrl+C를 누르면) 모니터링을 멈추고 txt 파일 저장!
        StopMonitoring();
    }
    catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        StopMonitoring(); // 에러로 꺼져도 보고서는 안전하게 뽑기
    }
    return 0;
}