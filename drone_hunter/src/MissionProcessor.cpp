#include "engine/MissionProcessor.h"
#include <fstream>
#include <string>
#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "interfaces/IConfigLoader.h"
#include "states/StateStopped.h"
#include "json.hpp"
#include "Log.h"

using json = nlohmann::json;

float length(Coord delta);

std::optional<SimStep> MissionProcessor::step() {

        float D = 0.f;
        int bestTarget = -1;
        float minTime = 1e9f;
        Coord targInterp;

        targets_->update(currentTime_);

        for (int i = 0; i < targetCount_; ++i) {

            targInterp = targets_->getPositionAt(i, currentTime_);

            Coord delta = targInterp - ctx_.pos;

            D = length(delta);
            float totalTime = (D - solver_->getHorizDist()) / config_.attackSpeed + solver_->getBallisticTime();

            Coord localPred;
            for (int k = 0; k < 3; k++)
            {
                localPred = targets_->getPositionAt(i, currentTime_ + totalTime);
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
        simStep.push_back(currentStep);

        Coord hitDiff = simStep.back().aimPoint - simStep.back().predictedTarget;
        if (hitDiff.x * hitDiff.x + hitDiff.y * hitDiff.y <= config_.hitRadius * config_.hitRadius)
        {
            targetHit_ = true;
            return simStep.back();
        }

        prevBestTarget_ = bestTarget;
        currentTime_ += config_.simTimeStep;
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
        out["steps"].push_back(step);
    }
    std::ofstream fout(path + "/simulation.json");
    fout << out.dump(2);
    fout.close();
}

void MissionProcessor::init() {

    targetCount_ = targets_->getTargetCount();
    config_ = loader_->getConfig();
    ammo_        = loader_->getAmmoParams();
    ctx_.cfg     = &config_;
    ctx_.pos     = config_.startPos;
    ctx_.direction = config_.initialDir;
    ctx_.speed   = 0.f;
    ctx_.angleDiff = 0.f;
    ctx_.accel   = config_.attackSpeed * config_.attackSpeed / (2.f * config_.accelPath);
    solver_->precompute(config_.attackSpeed, config_.altitude, ammo_);
    state_          = std::make_unique<StateStopped>();
    currentTime_    = 0.f;
    prevBestTarget_ = -1;
    simStep.clear();
    simStep.reserve(MAX_STEPS);
}

void MissionProcessor::reset() {
    simStep.clear();
    currentTime_ = 0;
    ctx_.pos = config_.startPos;
    ctx_.direction = config_.initialDir;
    ctx_.speed = 0.f;
    ctx_.angleDiff = 0.f;
    state_ = std::make_unique<StateStopped>();
    prevBestTarget_ = -1;
    targetHit_ = false;
}

float length(Coord delta)
{
    return sqrtf((delta.x) * (delta.x) + (delta.y) * (delta.y));
}
