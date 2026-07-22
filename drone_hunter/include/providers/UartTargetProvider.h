#pragma once
#include <atomic>
#include <mutex>
#include <vector>
#include "interfaces/ITargetProvider.h"
#include "link/UartLink.h"
#include "dto/Target.h"

class UartTargetProvider : public ITargetProvider {

public:
    UartTargetProvider(UartLink& link) : link_(link) {};
    void run() override;
    int getTargetCount() const override { return link_.getTargetCount(); }
    Target getTarget(int idx) const override;

    bool isThreadReady() const override { return ready_.load(); }
    void start() override { started_.store(true); }
    void stop() override;

    ~UartTargetProvider() override {}
private:
    UartLink& link_;
    std::atomic<bool> ready_{false};
    std::atomic<bool> started_{false};

    mutable std::vector<TargetHistory> targetHistory_;
    mutable std::mutex mtx_;

};