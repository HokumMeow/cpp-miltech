#pragma once
#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "interfaces/IConfigLoader.h"
#include "dto/DroneStates.h"
#include "dto/SimStep.h"

class MissionProcessor {
public:
    MissionProcessor(IBallisticSolver* s, ITargetProvider* t, IConfigLoader* l)
        : solver_(s), targets_(t), loader_(l) {}

    void init();
    bool hasNext() const { return currentIdx_ < targetCount_; }
    std::optional<SimStep> step();
    void reset() { currentIdx_ = 0; }
    void changeSolver(IBallisticSolver* s) { solver_ = s; }
private:
    void interpolate(float t, float arrayTimeStep, int targetIndex, int timeSteps , Coord** targets, Coord &output);
    float length(Coord delta);

    IBallisticSolver* solver_;
    ITargetProvider*  targets_;
    IConfigLoader*    loader_;
    Coord dronePos_;
    Target target_;
    AmmoParams ammo_;
    DroneState droneState_;
    DroneConfig config_;
    Coord bestPred_;

    SimStep* simStep = new SimStep[100000];
    const float PI = 3.14159265f;

    float accel_;
    int step_ = 0;
    int timeSteps_ = 0;
    int currentIdx_ = 0;
    int targetCount_ = 0;
    float currentDir_;
    float speed_ = 0.f;
    float currentTime_;
    float angleDiff_;
    int prevBestTarget_;

};