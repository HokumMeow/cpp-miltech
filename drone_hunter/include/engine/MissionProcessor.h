#pragma once
#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "interfaces/IConfigLoader.h"

class MissionProcessor {
public:
    MissionProcessor(IBallisticSolver* s, ITargetProvider* t, IConfigLoader* l)
        : solver_(s), targets_(t), loader_(l) {}

    void init(const char* cfgSource);
    bool hasNext() const { return currentIdx_ < targetCount_; }
    std::optional<DropPoint> step();
    void reset() { currentIdx_ = 0; }
    void changeSolver(IBallisticSolver* s) { solver_ = s; }
private:
    IBallisticSolver* solver_;     // ← НЕ AnalyticalSolver*, саме інтерфейс
    ITargetProvider*  targets_;
    IConfigLoader*    loader_;
    Coord dronePos_;
    float altitude_;
    AmmoParams ammo_;
    int currentIdx_ = 0;
    int targetCount_ = 0;
};