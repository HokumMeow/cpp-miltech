#pragma once
#include <netinet/in.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <mutex>
#include <string>

#include <common/mavlink.h>

#include "dto/Coord.h"

class IDronePhysics;

// telemetry MAVLink 2 over UDP 
class MavlinkLink {
public:
    MavlinkLink(IDronePhysics& physics, const std::string& host, uint16_t port, float altitude);
    ~MavlinkLink();

    void run();
    bool isThreadReady() const { return ready_.load(); }
    void start() { started_.store(true); }
    void stop() { stopFlag_.store(true); }
    void reportDrop(Coord dropPoint, float altitude);
    bool hasPendingDrop() const;

private:
    void sendHeartbeat();
    void sendTelemetry();
    void pollIncoming();
    void serviceDropRetries();
    void sendBuffer(const uint8_t* data, int len);

    IDronePhysics& physics_;
    int sock_;
    sockaddr_in destAddr_{};
    float altitude_;
    mavlink_status_t rxStatus_{};

    mutable std::mutex dropMtx_;
    bool dropPending_ = false;
    int dropAttempts_ = 0;
    Coord dropPoint_;
    float dropAlt_ = 0.f;
    std::chrono::steady_clock::time_point lastDropSend_;

    std::atomic<bool> ready_{false};
    std::atomic<bool> started_{false};
    std::atomic<bool> stopFlag_{false};

    static constexpr uint8_t kSysId = 1;
    static constexpr int kMaxDropAttempts = 5;
};
