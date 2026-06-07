#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <boost/asio.hpp> // endpoint를 사용하기 위해 추가

struct Player {
    uint32_t id;
    int hp;
    int score;
    float x, y;
    boost::asio::ip::udp::endpoint ep; // broadcast를 위해 ep 추가!

    Player() : id(0), hp(100), score(0), x(0), y(0) {}
};

// 플레이어 초기화 함수 선언
void InitPlayer(Player& p, uint32_t id, boost::asio::ip::udp::endpoint ep);

#endif