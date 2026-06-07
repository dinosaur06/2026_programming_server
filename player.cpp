#include "player.h"

void InitPlayer(Player& p, uint32_t id, boost::asio::ip::udp::endpoint ep) {
    p.id = id;
    p.hp = 100;        // 말씀하신 초기 체력 100 설정
    p.score = 0;
    p.x = 0.0f;
    p.y = 0.0f;
    p.ep = ep;         // 접속 주소 저장
}