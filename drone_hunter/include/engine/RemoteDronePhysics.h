#pragma once
#include <atomic>
#include <mutex>
#include "concurrency/ThreadSafeQueue.h"
#include "interfaces/IDronePhysics.h"
#include "link/UartLink.h"
#include "link/GpioLink.h"

class RemoteDronePhysics : public IDronePhysics {
public:
    RemoteDronePhysics(UartLink& link, GpioLink& gpio) : link_(link), gpio_(gpio) {};

    DroneTelemetry getTelemetry() const override;
    void sendCommand(const DroneCommand& cmd) override;

    void run() override;
    bool isThreadReady() const override { return ready_.load(); }
    void start() override { gpio_.raiseStart(); started_.store(true); }
    void stop() override { stopFlag_.store(true); }
    void drop() override { gpio_.pulseDrop(); }

private:
    UartLink& link_;
    GpioLink& gpio_;
    mutable std::mutex mtx_;

    std::atomic<bool> ready_{false};
    std::atomic<bool> started_{false};
    std::atomic<bool> stopFlag_{false};
};