#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <link/drone_link.h>

class UartLink {
    
public:
    UartLink(const std::string& port);
    
    dlink::Telemetry getTelemetry() const; 
    int getTargetCount() const;
    dlink::TargetPos getTarget(int idx) const;
    dlink::AmmoCfg getAmmoCfg() const; 
    dlink::DroneCfg getDroneCfg() const;

    void run();
    bool isThreadReady() const;
    void start();
    void stop();

    bool hasAmmo() const;
    bool hasConfig() const;
    bool hasTelemetry() const;

    void sendControl(float accel, float turnRate);

    ~UartLink();
private:
    int fd_;
    dlink::Parser parser_;
    mutable std::mutex mtx_;
    dlink::Telemetry telemetry_;
    std::vector<dlink::TargetPos> targets_;
    dlink::AmmoCfg ammoCfg_;
    dlink::DroneCfg droneCfg_;

    std::atomic<bool> hasAmmo_{false};
    std::atomic<bool> hasConfig_{false};
    std::atomic<bool> hasTelemetry_{false};
    std::atomic<bool> ready_{false};
    std::atomic<bool> started_{false};
    std::atomic<bool> stopFlag_{false};
};