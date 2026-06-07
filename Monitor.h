#ifndef MONITOR_H
#define MONITOR_H

#include <atomic>

// 전역 패킷 및 트래픽 카운터 (main.cpp에서 접근 가능하도록 extern 선언)
extern std::atomic<int> g_packetCount;
extern std::atomic<long long> g_totalBytes;

// 모니터링 제어 함수
void StartMonitoring();
void StopMonitoring();
void PrintFinalReport();

#endif // MONITOR_H