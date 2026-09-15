#include "engine/MissionProcessor.h"
#include <chrono>
#include <cmath>
#include <fstream>
#include <thread>
#include "dto/DroneTelemetry.h"
#include "interfaces/IBallisticSolver.h"
#include "interfaces/IConfigLoader.h"
#include "interfaces/IDronePhysics.h"
#include "interfaces/ITargetProvider.h"
#include "states/StateStopped.h"
#include "telemetry/MavlinkLink.h"
#include "json.hpp"
#include "Log.h"

using json = nlohmann::json;

float length(Coord delta);

std::optional<SimStep> MissionProcessor::step() {

        DroneTelemetry tel = physics_.getTelemetry();
        ctx_.pos       = tel.pos;
        ctx_.direction = tel.direction;
        ctx_.speed     = std::hypot(tel.speed.x, tel.speed.y);

        float D = 0.f;
        int bestTarget = -1;
        float minTime = 1e9f;
        Coord targInterp;

        for (int i = 0; i < targetCount_; ++i) {

            Target tgt = targets_.getTarget(i);
            targInterp = tgt.pos;

            Coord delta = targInterp - ctx_.pos;

            D = length(delta);
            float totalTime = (D - solver_->getHorizDist()) / config_.attackSpeed + solver_->getBallisticTime();

            Coord localPred;
            for (int k = 0; k < 3; k++)
            {
                localPred = tgt.pos + tgt.velocity * totalTime;
                delta = localPred - ctx_.pos;
                D = length(delta);
                totalTime = (D - solver_->getHorizDist()) / config_.attackSpeed + solver_->getBallisticTime();
            }

            float timeToStop = 0.f;
            if (i != prevBestTarget_)
            {
                timeToStop = state_->timeToStop(ctx_);
            }

            if (totalTime + timeToStop < minTime)
            {
                minTime = totalTime + timeToStop;
                bestTarget = i;
                bestPred_ = localPred;
            }
        }

        DEBUG("  target=" << bestTarget << " state=" << state_->name());

        std::optional<Coord> firePoint = solver_->solve(ctx_.pos, bestPred_, config_.attackSpeed, config_.altitude, ammo_);

        if (!firePoint.has_value())
        {
            return std::nullopt;
        }

        ctx_.angleToTarget = atan2f(firePoint->y - ctx_.pos.y, firePoint->x - ctx_.pos.x);

        ctx_.angleDiff = ctx_.angleToTarget - ctx_.direction;

        while (ctx_.angleDiff > PI)
            ctx_.angleDiff -= 2 * PI;
        while (ctx_.angleDiff < -PI)
            ctx_.angleDiff += 2 * PI;

        if (auto interrupted = state_->interrupt(ctx_)) {
            state_ = std::move(interrupted);
        }

        auto next = state_->execute(ctx_);
        if (next) state_ = std::move(next);

        physics_.sendCommand(state_->command(ctx_));

        DEBUG("Step " << simStep.size() << " pos=(" << ctx_.pos.x << "," << ctx_.pos.y << ")");

        Coord dir = { cosf(ctx_.direction), sinf(ctx_.direction) };

        SimStep currentStep;

        currentStep.pos = ctx_.pos;
        currentStep.direction = ctx_.direction;
        currentStep.state = state_->name();
        currentStep.targetIdx = bestTarget;
        currentStep.dropPoint = firePoint;
        currentStep.aimPoint = ctx_.pos + dir * solver_->getHorizDist();
        currentStep.predictedTarget = bestPred_;
        currentStep.timeSecSinceStart = tel.timeSecSinceStart;
        simStep.push_back(currentStep);

        Coord hitDiff = simStep.back().aimPoint - simStep.back().predictedTarget;
        if (hitDiff.x * hitDiff.x + hitDiff.y * hitDiff.y <= config_.hitRadius * config_.hitRadius)
        {
            targetHit_ = true;
            return simStep.back();
        }

        prevBestTarget_ = bestTarget;
        return std::nullopt;
}

void MissionProcessor::saveResults(const std::string& path)
{
    json out;
    out["totalSteps"] = simStep.size();
    out["steps"] = json::array();

    for (const auto& s : simStep){
        json step;
        step["position"]        = {{"x", s.pos.x}, {"y", s.pos.y}};
        step["direction"]       = s.direction;
        step["state"]           = s.state;
        step["targetIndex"]     = s.targetIdx;
        step["dropPoint"]       = {{"x", s.dropPoint->x},
                                {"y", s.dropPoint->y}};
        step["aimPoint"]        = {{"x", s.aimPoint.x},
                                {"y", s.aimPoint.y}};
        step["predictedTarget"] = {{"x", s.predictedTarget.x},
                                {"y", s.predictedTarget.y}};
        step["timeSecSinceStart"] = s.timeSecSinceStart;
        out["steps"].push_back(step);
    }
    std::ofstream fout(path + "/simulation.json");
    fout << out.dump(2);
    fout.close();
}

void MissionProcessor::init() {

    targetCount_ = targets_.getTargetCount();
    config_ = loader_->getConfig();
    ammo_        = loader_->getAmmoParams();
    ctx_.cfg     = &config_;
    ctx_.accel   = config_.attackSpeed * config_.attackSpeed / (2.f * config_.accelPath);
    solver_->precompute(config_.attackSpeed, config_.altitude, ammo_);
    state_          = std::make_unique<StateStopped>();
    prevBestTarget_ = -1;
    simStep.clear();
    simStep.reserve(MAX_STEPS);
}

void MissionProcessor::run() {
    init();
    ready_.store(true);
    while (!started_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    while (hasNext()) {
        auto result = step();
        if (result.has_value()) {
            LOG("Hit! drop at (" << result->dropPoint->x << ", " << result->dropPoint->y << ")");
            physics_.drop();
            if (mavlink_) {
                mavlink_->reportDrop(*result->dropPoint, config_.altitude);
            }
            break;
        }
        std::this_thread::sleep_for(std::chrono::duration<float>(config_.simTimeStep / config_.timeScale));
    }

    LOG("Simulation finished");
}

float length(Coord delta)
{
    return sqrtf((delta.x) * (delta.x) + (delta.y) * (delta.y));
}
