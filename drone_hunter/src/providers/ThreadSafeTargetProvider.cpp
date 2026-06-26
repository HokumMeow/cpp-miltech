#include "providers/ThreadSafeTargetProvider.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include "json.hpp"

using json = nlohmann::json;

ThreadSafeTargetProvider::ThreadSafeTargetProvider(const std::string& path, float arrayTimeStep,
                                                     float targetTimeStep, float timeScale)
    : arrayTimeStep_(arrayTimeStep), targetTimeStep_(targetTimeStep), timeScale_(timeScale) {
    std::ifstream ft(path + "/targets.json");
    json jt;
    ft >> jt;
    targetCount_ = jt["targetCount"];
    timeSteps_ = jt["timeSteps"];
    if (targetCount_ <= 0) {
        std::cerr << "0 targets!" << std::endl;
        return;
    }

    trajectories_ = std::vector<std::vector<Coord>>(targetCount_, std::vector<Coord>(timeSteps_));
    for (int i = 0; i < targetCount_; i++) {
        for (int j = 0; j < timeSteps_; j++) {
            trajectories_[i][j].x = jt["targets"][i]["positions"][j]["x"];
            trajectories_[i][j].y = jt["targets"][i]["positions"][j]["y"];
        }
    }
    current_ = std::vector<Target>(targetCount_);
    advance(0.f);

    thread_ = std::thread(&ThreadSafeTargetProvider::run, this);
}

ThreadSafeTargetProvider::~ThreadSafeTargetProvider() {
    stop();
}

void ThreadSafeTargetProvider::advance(float simTime) {
    std::lock_guard<std::mutex> lk(mtx_);
    for (int i = 0; i < targetCount_; i++) {
        int idx = static_cast<int>(std::floor(simTime / arrayTimeStep_)) % timeSteps_;
        int next = (idx + 1) % timeSteps_;
        float frac = (simTime - idx * arrayTimeStep_) / arrayTimeStep_;

        const Coord& a = trajectories_[i][idx];
        const Coord& b = trajectories_[i][next];

        current_[i].pos.x = a.x + (b.x - a.x) * frac;
        current_[i].pos.y = a.y + (b.y - a.y) * frac;
        current_[i].velocity.x = (b.x - a.x) / arrayTimeStep_;
        current_[i].velocity.y = (b.y - a.y) / arrayTimeStep_;
    }
}

void ThreadSafeTargetProvider::run() {
    ready_.store(true);
    while (!started_.load() && !stopFlag_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    float simTime = 0.f;
    while (!stopFlag_.load()) {
        simTime += targetTimeStep_;
        advance(simTime);
        std::this_thread::sleep_for(std::chrono::duration<float>(targetTimeStep_ / timeScale_));
    }
}

Target ThreadSafeTargetProvider::getTarget(int idx) const {
    std::lock_guard<std::mutex> lk(mtx_);
    return current_.at(idx);
}

void ThreadSafeTargetProvider::stop() {
    stopFlag_.store(true);
    if (thread_.joinable()) thread_.join();
}
