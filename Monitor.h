#pragma once
#include <atomic>
#include <string>

// 유니티가 핑 패킷을 보낼 때마다 서버에서 카운트를 올릴 변수들
extern std::atomic<int> g_pingPacketCount;       // 1초 동안 들어온 PING 패킷 수
extern std::atomic<long long> g_totalTrafficBytes; // 1초 동안 들어온 순수 트래픽 양

// 서버 전체 누적 통계용 변수 (보고서용)
extern std::atomic<int> g_grandTotalPackets;

// 모니터링 시작 및 종료 함수
void StartMonitoring();
void StopMonitoring();