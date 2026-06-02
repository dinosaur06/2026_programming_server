#include <boost/asio.hpp>
#include <iostream>
#include <unordered_map>
#include "PacketTypes.h"

using namespace boost::asio;
using udp = ip::udp;

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
            char pong = static_cast<char>(PacketType::PING);
            socket_.async_send_to(buffer(&pong, 1), remote_ep_,
                [](boost::system::error_code, std::size_t) {});
        }
    }

    void broadcast(const char* data, std::size_t len, uint32_t exclude_id) {
        for (auto& [id, ep] : clients_) {
            if (id == exclude_id) continue;
            socket_.async_send_to(buffer(data, len), ep,
                [](boost::system::error_code, std::size_t) {});
        }
    }
};

int main() {
    io_context ctx;
    FpsServer server(ctx, 9000);
    ctx.run(); // 이벤트 루프 (블로킹)
    return 0;
}
