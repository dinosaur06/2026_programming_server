#include "Monitor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <fstream>
#include <iomanip>

using namespace std;

// 전역 변수 초기화
atomic<int> g_pingPacketCount(0);
atomic<long long> g_totalTrafficBytes(0);
atomic<int> g_grandTotalPackets(0);

static bool g_keepRunning = true;
static thread g_monitorThread;

// 보고서 작성을 위한 1초 주기 통계 데이터 구조체
struct GameServerStats {
    int second;      // 서버 가동 후 경과 시간(초)
    int pps;         // Packet Per Second (초당 패킷 수)
    double kbps;     // Kilobytes Per Second (초당 트래픽 바이트)
};
static vector<GameServerStats> g_history;

void MonitorLoop() {
    int elapsedSeconds = 0;
    int maxPPS = 0;
    int maxPPSAt = 0;

    while (g_keepRunning) {
        this_thread::sleep_for(chrono::seconds(1));
        if (!g_keepRunning) break;

        // 1초 동안 쌓인 데이터 원자적으로 가져오고 초기화
        int currentPPS = g_pingPacketCount.exchange(0);
        long long currentBytes = g_totalTrafficBytes.exchange(0);
        double currentKB = currentBytes / 1024.0;

        // 누적 총 패킷 수 증가
        g_grandTotalPackets += currentPPS;

        // 피크치(최대 PPS) 계산
        if (currentPPS > maxPPS) {
            maxPPS = currentPPS;
            maxPPSAt = elapsedSeconds;
        }

        // 히스토리에 매초 기록 저장
        g_history.push_back({ elapsedSeconds, currentPPS, currentKB });
        elapsedSeconds++;
    }

    // [서버 종료 시점] txt 보고서 저장 로직 실행
    ofstream outFile("network_report.txt");
    if (outFile.is_open()) {
        outFile << "========================================" << endl;
        outFile << "       Game Server Network Report       " << endl;
        outFile << "========================================" << endl;
        outFile << "1. Total Game Packets Received : " << g_grandTotalPackets << " pkts" << endl;
        outFile << "2. Peak PPS Value              : " << maxPPS << " pkts/s (At: " << maxPPSAt << "s)" << endl;
        outFile << "3. Total Runtime               : " << elapsedSeconds << " seconds" << endl;
        outFile << "----------------------------------------" << endl;

        // 지연 유저나 트래픽 폭발 의심 시점 자동 필터링 (예: PPS가 50을 넘었을 때)
        outFile << "\n[Anomalous Traffic Check (Over 50 PPS)]" << endl;
        bool foundAnomaly = false;
        for (const auto& s : g_history) {
            if (s.pps > 50) {
                outFile << " - [Warning at " << s.second << "s] High Traffic Detected -> PPS: " << s.pps << " / Traffic: " << fixed << setprecision(2) << s.kbps << " KB/s" << endl;
                foundAnomaly = true;
            }
        }
        if (!foundAnomaly) {
            outFile << " - No unusual traffic congestion detected." << endl;
        }

        // 전체 시간별 텍스트 타임라인 테이블 출력
        outFile << "\n[Full Session Timeline Data]" << endl;
        outFile << "Time(s) |  PPS  | Traffic(KB/s)" << endl;
        outFile << "----------------------------------------" << endl;
        for (const auto& s : g_history) {
            outFile << setw(7) << s.second << " | "
                << setw(5) << s.pps << " | "
                << setw(13) << fixed << setprecision(2) << s.kbps << endl;
        }
        outFile << "========================================" << endl;
        outFile.close();

        cout << "\n[+] Network report successfully saved to 'network_report.txt'." << endl;
    }
    g_history.clear();
}

void StartMonitoring() {
    g_keepRunning = true;
    g_monitorThread = thread(MonitorLoop);
    cout << "[+] Server Telemetry Monitoring Started." << endl;
}

void StopMonitoring() {
    g_keepRunning = false;
    if (g_monitorThread.joinable()) {
        g_monitorThread.join();
    }
    cout << "[-] Server Telemetry Monitoring Stopped." << endl;
}