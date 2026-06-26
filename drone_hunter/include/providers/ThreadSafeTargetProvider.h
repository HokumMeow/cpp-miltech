#pragma once
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include "interfaces/ITargetProvider.h"

class ThreadSafeTargetProvider : public ITargetProvider {
public:
    ThreadSafeTargetProvider(const std::string& path, float arrayTimeStep,
                              float targetTimeStep, float timeScale);
    ~ThreadSafeTargetProvider() override;

    int getTargetCount() const override { return targetCount_; }
    Target getTarget(int idx) const override;

    bool isThreadReady() const override { return ready_.load(); }
    void start() override { started_.store(true); }
    void stop() override;

private:
    void run();
    void advance(float simTime);

    // Приватні дані провайдера — траєкторії з targets.json. Назовні не видно.
    std::vector<std::vector<Coord>> trajectories_;
    int targetCount_ = 0;
    int timeSteps_ = 0;
    float arrayTimeStep_;
    float targetTimeStep_;
    float timeScale_;

    mutable std::mutex mtx_;
    std::vector<Target> current_;

    std::atomic<bool> ready_{false};
    std::atomic<bool> started_{false};
    std::atomic<bool> stopFlag_{false};
    std::thread thread_;
};
