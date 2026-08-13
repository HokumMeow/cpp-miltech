#include "providers/ThreadSafeTargetProvider.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <thread>
#include "json.hpp"

using json = nlohmann::json;

ThreadSafeTargetProvider::ThreadSafeTargetProvider(const std::string& path, float arrayTimeStep,
                                                     float targetTimeStep, float timeScale)
    : arrayTimeStep_(arrayTimeStep), targetTimeStep_(targetTimeStep), timeScale_(timeScale) {
    std::ifstream ft(path + "/targets.json");
    json jt;
    ft >> jt;
    targetCount_ = jt["targetCount"];
    timeSteps_   = jt["timeSteps"];
    if (targetCount_ <= 0) {
        std::cerr << "0 targets!" << std::endl;
        return;
    }

    trajectories_.assign(targetCount_, std::vector<Coord>(timeSteps_));
    for (int i = 0; i < targetCount_; i++) {
        for (int j = 0; j < timeSteps_; j++) {
            trajectories_[i][j].x = jt["targets"][i]["positions"][j]["x"];
            trajectories_[i][j].y = jt["targets"][i]["positions"][j]["y"];
        }
    }

    advance(0.f);
    
}

void ThreadSafeTargetProvider::advance(float simTime) {
    std::vector<Target> updated(targetCount_);
    for (int i = 0; i < targetCount_; i++) {
        int idx     = static_cast<int>(std::floor(simTime / arrayTimeStep_)) % timeSteps_;
        int nextIdx = (idx + 1) % timeSteps_;

        float frac = fmod(simTime, arrayTimeStep_) / arrayTimeStep_;
        updated[i].pos = trajectories_[i][idx] + (trajectories_[i][nextIdx] - trajectories_[i][idx]) * frac;
 
        updated[i].velocity.x = (trajectories_[i][nextIdx].x - trajectories_[i][idx].x) / arrayTimeStep_;
        updated[i].velocity.y = (trajectories_[i][nextIdx].y - trajectories_[i][idx].y) / arrayTimeStep_;
    }
    std::lock_guard<std::mutex> lk(mtx_);
    current_ = std::move(updated);
}

void ThreadSafeTargetProvider::run() {
    ready_.store(true);
    while (!started_.load() && !stopFlag_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    float simTime = 0.f;
    while (!stopFlag_.load()) {
        advance(simTime);
        std::this_thread::sleep_for(std::chrono::duration<float>(targetTimeStep_ / timeScale_));
        simTime += targetTimeStep_;
    }
}

Target ThreadSafeTargetProvider::getTarget(int idx) const {
    std::lock_guard<std::mutex> lk(mtx_);
    return current_.at(idx);
}

void ThreadSafeTargetProvider::stop() {
    stopFlag_.store(true);
}
