#include <iostream>
#include <thread>
#include "providers/UartTargetProvider.h"

void UartTargetProvider::run() {
    ready_.store(true);
    while (!started_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    
}

Target UartTargetProvider::getTarget(int idx) const {

    dlink::TargetPos raw = link_.getTarget(idx);

    std::lock_guard<std::mutex> lk(mtx_);
    if (idx >= static_cast<int>(targetHistory_.size())) {
        targetHistory_.resize(idx + 1);
    }

    Target t;
    auto now = std::chrono::steady_clock::now();

    if (!targetHistory_[idx].seen){
        t.velocity = {0.0f, 0.0f};
    } else {
        auto dt = std::chrono::duration<float>(now - targetHistory_[idx].lastUpdate).count();
        if (dt > 0.f) {
            Coord pos;
            pos.x = raw.x;
            pos.y = raw.y;     
            t.velocity = (pos - targetHistory_[idx].pos) / dt;
        } else {
            t.velocity = {0.0f, 0.0f};
        }
    }

    targetHistory_[idx].pos.x = raw.x;
    targetHistory_[idx].pos.y = raw.y;
    targetHistory_[idx].lastUpdate = now;
    targetHistory_[idx].seen = true;
    
    t.pos = targetHistory_[idx].pos;

    return t;
}

void UartTargetProvider::stop() {} 