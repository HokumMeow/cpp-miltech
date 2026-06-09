#include "engine/MissionProcessor.h"
#include <fstream>
#include <string>
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
            
            Coord delta = targInterp - dronePos_;

            D = length(delta);
            float totalTime = (D - solver_->getHorizDist()) / config_.attackSpeed + solver_->getBallisticTime();

            Coord localPred;
            for (int k = 0; k < 3; k++)
            {
                localPred = targets_->getPositionAt(i, currentTime_ + totalTime);
                delta = localPred - dronePos_;
                D = length(delta);
                totalTime = (D - solver_->getHorizDist()) / config_.attackSpeed + solver_->getBallisticTime();
            }            

            float timeToStop = 0.f;
            if (i != prevBestTarget_)
            {
                switch (droneState_)
                {
                case STOPPED:
                    timeToStop = 0.f;
                    break;
                case ACCELERATING:
                    timeToStop = speed_ / accel_;
                    break;
                case DECELERATING:
                    timeToStop = speed_ / accel_;
                    break;
                case MOVING:
                    timeToStop = config_.attackSpeed / accel_;
                    break;
                case TURNING:
                    timeToStop = fabsf(angleDiff_) / config_.angularSpeed;
                    break;
                }
            }

            if (totalTime + timeToStop < minTime)
            {
                minTime = totalTime + timeToStop;
                bestTarget = i;
                bestPred_ = localPred;
            }            
        }
        
        DEBUG("  target=" << bestTarget << " state=" << droneState_);

        std::optional<Coord> firePoint = solver_->solve(dronePos_, bestPred_, config_.attackSpeed, config_.altitude, ammo_);

        if (!firePoint.has_value())
        {
            return std::nullopt;
        }

        float angleToTarget = atan2f(firePoint->y - dronePos_.y, firePoint->x - dronePos_.x);
 
        angleDiff_ = angleToTarget - currentDir_;

        while (angleDiff_ > PI)
            angleDiff_ -= 2 * PI;
        while (angleDiff_ < -PI)
            angleDiff_ += 2 * PI;

        if (fabsf(angleDiff_) > config_.turnThreshold)
        {
            switch (droneState_)
            {
            case STOPPED:
                droneState_ = TURNING;
                break;
            case MOVING:
                droneState_ = DECELERATING;
                break;
            }
        }

        switch (droneState_)
        {
        case STOPPED:
            if (fabsf(angleDiff_) < config_.turnThreshold)
            {
                droneState_ = ACCELERATING;
                angleDiff_ = 0.f;
            }
            else
            {
                droneState_ = TURNING;
            }
            break;
        case ACCELERATING:
            speed_ += accel_ * config_.simTimeStep;
            if (speed_ >= config_.attackSpeed)
            {
                speed_ = config_.attackSpeed;
                droneState_ = MOVING;
            }
            dronePos_.x += speed_ * cosf(currentDir_) * config_.simTimeStep;
            dronePos_.y += speed_ * sinf(currentDir_) * config_.simTimeStep;
            break;

        case DECELERATING:
            speed_ -= accel_ * config_.simTimeStep;
            if (speed_ <= 0.f)
            {
                speed_ = 0.f;
                droneState_ = TURNING;
            }
            dronePos_.x += speed_ * cosf(currentDir_) * config_.simTimeStep;
            dronePos_.y += speed_ * sinf(currentDir_) * config_.simTimeStep;
            break;
        case MOVING:
            if (fabsf(angleDiff_) > config_.turnThreshold)
            {
                droneState_ = DECELERATING;
            }
            else
            {
                if (fabsf(angleDiff_) < config_.turnThreshold)
                {
                    currentDir_ = angleToTarget;
                }
                dronePos_.x += speed_ * cosf(currentDir_) * config_.simTimeStep;
                dronePos_.y += speed_ * sinf(currentDir_) * config_.simTimeStep;
            }
            break;
        case TURNING:
        {
            float turnStep = config_.angularSpeed * config_.simTimeStep;
            if (fabsf(angleDiff_) <= turnStep)
            {
                currentDir_ = angleToTarget;
                droneState_ = ACCELERATING;
                angleDiff_ = 0.f;
            }
            else
            {
                currentDir_ += (angleDiff_ > 0 ? 1.f : -1.f) * turnStep;
            }
            break;
        }
        }

        DEBUG("Step " << simStep.size() << " pos=(" << dronePos_.x << "," << dronePos_.y << ")");

        Coord dir = { cosf(currentDir_), sinf(currentDir_) };

        SimStep currentStep;

        currentStep.pos = dronePos_;
        currentStep.direction = currentDir_;
        currentStep.state = droneState_;
        currentStep.targetIdx = bestTarget;
        currentStep.dropPoint = firePoint;
        currentStep.aimPoint = dronePos_ + dir * solver_->getHorizDist();
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
    dronePos_    = config_.startPos;
    currentDir_ = config_.initialDir;
    accel_ = config_.attackSpeed * config_.attackSpeed / (2.f * config_.accelPath);
    solver_->precompute(config_.attackSpeed, config_.altitude, ammo_);
    droneState_     = STOPPED;
    currentTime_    = 0.f;
    angleDiff_      = 0.f;
    prevBestTarget_ = -1;
    simStep.clear();
    simStep.reserve(MAX_STEPS);
}

float length(Coord delta)
{
    return sqrtf((delta.x) * (delta.x) + (delta.y) * (delta.y));
}
