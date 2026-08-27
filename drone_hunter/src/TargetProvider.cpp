#include "engine/TargetProvider.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include "json.hpp"

using json = nlohmann::json;

JsonTargetProvider::JsonTargetProvider(const char* path, const float arrayTimeStep){

    std::ifstream ft(path + std::string("/targets.json"));
    json jt; ft >> jt;
    targetCount_ = jt["targetCount"];
    timeSteps_ = jt["timeSteps"];
    arrayTimeStep_ = arrayTimeStep;
    targets = nullptr;
    if (targetCount_ <= 0) {
        std::cerr << "0 targets!" << std::endl;
        return;
    }
    targets = new Coord*[targetCount_];
    for (int i = 0; i < targetCount_; i++) {
        targets[i] = new Coord[timeSteps_];
        for (int j = 0; j < timeSteps_; j++) {
            targets[i][j].x = jt["targets"][i]["positions"][j]["x"];
            targets[i][j].y = jt["targets"][i]["positions"][j]["y"];
        }
    }
    current_ = new Target[targetCount_]{};

};

void JsonTargetProvider::update(float time) {
    for (int i = 0; i < targetCount_; i++) {
        int idx = (int)floorf(time / arrayTimeStep_) % timeSteps_;
        int next = (idx + 1) % timeSteps_;
        float frac = (time - idx * arrayTimeStep_) / arrayTimeStep_;
        current_[i].pos.x = targets[i][idx].x + (targets[i][next].x - targets[i][idx].x) * frac;
        current_[i].pos.y = targets[i][idx].y + (targets[i][next].y - targets[i][idx].y) * frac;
        current_[i].velocity.x = (targets[i][next].x - targets[i][idx].x) / arrayTimeStep_;
        current_[i].velocity.y = (targets[i][next].y - targets[i][idx].y) / arrayTimeStep_;
    }
};

Coord JsonTargetProvider::getPositionAt(int idx, float time) {
    int idx_ = (int)floorf(time / arrayTimeStep_) % timeSteps_;
    int next = (idx_ + 1) % timeSteps_;
    float frac = (time - idx_ * arrayTimeStep_) / arrayTimeStep_;
    return {
        targets[idx][idx_].x + (targets[idx][next].x - targets[idx][idx_].x) * frac,
        targets[idx][idx_].y + (targets[idx][next].y - targets[idx][idx_].y) * frac
    };
}

JsonTargetProvider::~JsonTargetProvider() {
    for (int i = 0; i < targetCount_; i++)
    delete[] targets[i];
    delete[] targets;
    delete[] current_;
}
