#pragma once
#include <memory>
#include <vector>
#include <string>
#include "dto/AmmoParams.h"
#include "dto/DroneConfig.h"
#include "dto/DroneContext.h"
#include "dto/SimStep.h"
#include "states/IDroneState.h"

// Forward declarations for interfaces
class ITargetProvider;
class IBallisticSolver;
class IConfigLoader;

class MissionProcessor {
public:
    MissionProcessor(
        std::unique_ptr<IBallisticSolver> s,
        std::unique_ptr<ITargetProvider> t,
        std::unique_ptr<IConfigLoader> c)
        : solver_(std::move(s)),
          targets_(std::move(t)),
          loader_(std::move(c)) {}

    void init();
    bool hasNext() const { return !targetHit_ && simStep.size() < MAX_STEPS; }
    std::optional<SimStep> step();
    void reset();
    void changeSolver(std::unique_ptr<IBallisticSolver> s) { solver_ = std::move(s); }
    void saveResults(const std::string& path);
private:

    std::unique_ptr<IBallisticSolver> solver_;
    std::unique_ptr<ITargetProvider> targets_;
    std::unique_ptr<IConfigLoader> loader_;
    std::unique_ptr<IDroneState> state_;
    DroneContext ctx_;
    AmmoParams ammo_;
    DroneConfig config_;
    Coord bestPred_;
    static constexpr int MAX_STEPS = 10000;
    std::vector<SimStep> simStep;
    static constexpr float PI = 3.14159265f;
    int targetCount_ = 0;
    float currentTime_;
    int prevBestTarget_;
    bool targetHit_ = false;
};
