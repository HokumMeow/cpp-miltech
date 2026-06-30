#pragma once
#include <vector>
#include <string>
#include "dto/AmmoParams.h"
#include "dto/DroneConfig.h"
#include "dto/DroneStates.h"
#include "dto/SimStep.h"

class IBallisticSolver;
class ITargetProvider;
class IConfigLoader;

class MissionProcessor {
public:
    MissionProcessor(IBallisticSolver* s, ITargetProvider* t, IConfigLoader* l)
        : solver_(s), targets_(t), loader_(l) {}

    void init();
    bool hasNext() const { return !targetHit_ && simStep.size() < MAX_STEPS; }
    std::optional<SimStep> step();
    void reset() {
        simStep.clear();
        currentTime_ = 0;
        dronePos_ = config_.startPos;
        droneState_ = STOPPED;
        speed_ = 0.f;
        currentDir_ = config_.initialDir;
        prevBestTarget_ = -1;
        targetHit_ = false;
        angleDiff_ = 0.f;
    }
    void changeSolver(IBallisticSolver* s) { solver_ = s; }
    void saveResults(const std::string& path);
    ~MissionProcessor() { }
private:
    
    IBallisticSolver* solver_;
    ITargetProvider*  targets_;
    IConfigLoader*    loader_;
    Coord dronePos_;
    AmmoParams ammo_;
    DroneState droneState_;
    DroneConfig config_;
    Coord bestPred_;
    static constexpr int MAX_STEPS = 10000;
    std::vector<SimStep> simStep;
    static constexpr float PI = 3.14159265f;
    float accel_;
    int targetCount_ = 0;
    float currentDir_;
    float speed_ = 0.f;
    float currentTime_;
    float angleDiff_;
    int prevBestTarget_;
    bool targetHit_ = false;
};