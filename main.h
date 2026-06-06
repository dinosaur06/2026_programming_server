#pragma once
#include <boost/asio.hpp>
#include <unordered_map>
#include <cstdint>

using namespace boost::asio;
using udp = ip::udp;

class FpsServer {
public:
    FpsServer(io_context& ctx, uint16_t port);
    void broadcast(const char* data, std::size_t len, uint32_t exclude_id);

private:
    void do_receive();
    void handle_packet(std::size_t bytes);

    udp::socket  socket_;
    udp::endpoint remote_ep_;
    char          recv_buf_[1024];
    std::unordered_map<uint32_t, udp::endpoint> clients_;
};