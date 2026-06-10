#include "Monitor.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <vector>
#include <fstream>
#include <iomanip>

using namespace std;

// 전역 변수 초기화
atomic<int> g_packetCount(0);
atomic<long long> g_totalBytes(0);
static bool g_keepRunning = true;

struct Stats {
    int second;
    int pps;
    double kbps;
};
vector<Stats> g_history;

void MonitorLoop() {
    int elapsedSeconds = 0;
    while (g_keepRunning) {
        this_thread::sleep_for(chrono::seconds(1));

        // 1초간 모인 데이터 추출 후 카운터 리셋
        int currentCount = g_packetCount.exchange(0);
        long long currentBytes = g_totalBytes.exchange(0);
        double currentKB = currentBytes / 1024.0;

        g_history.push_back({ elapsedSeconds, currentCount, currentKB });

        // 초당 1000 패킷 이상 유입 시 디도스 의심 경고
        if (currentCount > 1000) {
            cout << "\a[!] Warning: Packet per second spike at [" << elapsedSeconds << "s]. PPS: " << currentCount << endl;
        }
        elapsedSeconds++;
    }
}

void PrintFinalReport() {
    long long grandTotalPackets = 0;
    int maxPPS = 0;
    int maxPPSAt = 0;
    int threshold = 500; // 탐지 임계치
    vector<int> thresholdReachedTimes;

    for (const auto& s : g_history) {
        grandTotalPackets += s.pps;
        if (s.pps > maxPPS) {
            maxPPS = s.pps;
            maxPPSAt = s.second;
        }
        if (s.pps >= threshold) {
            thresholdReachedTimes.push_back(s.second);
        }
    }

    // 콘솔 요약 출력
    cout << "\n" << string(40, '=') << endl;
    cout << "         [ Server Security Analysis Results ]" << endl;
    cout << string(40, '=') << endl;
    cout << " - Server Uptime: " << g_history.size() << " sec" << endl;
    cout << " - Total Packets Received: " << grandTotalPackets << endl;
    cout << " - Peak Value: " << maxPPS << " PPS (At: " << maxPPSAt << " sec)" << endl;
    cout << " - Anomalies Detected: " << (thresholdReachedTimes.empty() ? 0 : thresholdReachedTimes.size()) << " 회" << endl;
    cout << string(40, '=') << endl;

    // 파일 저장
    ofstream outFile("security_report.txt");
    if (outFile.is_open()) {
        outFile << "========================================" << endl;
        outFile << "       Server Security Detailed Report" << endl;
        outFile << "========================================" << endl;
        outFile << "1. Total Packets Received: " << grandTotalPackets << " 개" << endl;
        outFile << "2. Peak PPS Value: " << maxPPS << " pkts/s (At: " << maxPPSAt << " sec)" << endl;
        outFile << "\n3. Threshold (" << threshold << " PPS) Exceedance Records:" << endl;

        if (thresholdReachedTimes.empty()) {
            outFile << " - No anomalies detected" << endl;
        }
        else {
            for (int t : thresholdReachedTimes) {
                outFile << " - [At " << t << "s] Threshold exceeded" << endl;
            }
        }

        outFile << "\n[Full Timeline Data]" << endl;
        outFile << "Time(s) | PPS | Traffic(KB/s)" << endl;
        for (const auto& s : g_history) {
            outFile << setw(6) << s.second << " | " << setw(5) << s.pps << " | " << s.kbps << endl;
        }
        outFile.close();
        cout << "\n[+] Report successfully saved to 'security_report.txt'." << endl;
    }
    g_history.clear();
}

void StartMonitoring() {
    g_keepRunning = true;
    thread t(MonitorLoop);
    t.detach();
}

void StopMonitoring() {
    g_keepRunning = false;
    this_thread::sleep_for(chrono::milliseconds(500));
    PrintFinalReport();
}
