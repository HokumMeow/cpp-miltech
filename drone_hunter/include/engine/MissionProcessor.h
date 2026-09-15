#pragma once
#include <atomic>
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
class IDronePhysics;
class IBallisticSolver;
class IConfigLoader;
class MavlinkLink;

class MissionProcessor {
public:
    MissionProcessor(
        std::unique_ptr<IBallisticSolver> s,
        std::unique_ptr<IConfigLoader> c,
        ITargetProvider& targets,
        IDronePhysics& physics,
        MavlinkLink* mavlink = nullptr)
        : solver_(std::move(s)),
          loader_(std::move(c)),
          targets_(targets),
          physics_(physics),
          mavlink_(mavlink) {}

    void init();
    bool hasNext() const { return !targetHit_ && simStep.size() < MAX_STEPS; }
    std::optional<SimStep> step();
    void run();
    bool isThreadReady() const { return ready_.load(); }
    void start() { started_.store(true); }
    void changeSolver(std::unique_ptr<IBallisticSolver> s) { solver_ = std::move(s); }
    void saveResults(const std::string& path);
private:

    std::unique_ptr<IBallisticSolver> solver_;
    std::unique_ptr<IConfigLoader> loader_;
    ITargetProvider& targets_;
    IDronePhysics& physics_;
    MavlinkLink* mavlink_;
    std::unique_ptr<IDroneState> state_;
    DroneContext ctx_;
    AmmoParams ammo_;
    DroneConfig config_;
    Coord bestPred_;
    static constexpr int MAX_STEPS = 10000;
    std::vector<SimStep> simStep;
    static constexpr float PI = 3.14159265f;
    int targetCount_ = 0;
    int prevBestTarget_ = -1;
    bool targetHit_ = false;
    std::atomic<bool> ready_{false};
    std::atomic<bool> started_{false};
};
