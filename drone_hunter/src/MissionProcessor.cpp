#include "engine/MissionProcessor.h"
#include <fstream>
#include "json.hpp"

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

        DEBUG("Step " << step_ << " pos=(" << dronePos_.x << "," << dronePos_.y << ")");

        Coord dir = { cosf(currentDir_), sinf(currentDir_) };
                
        simStep[step_].pos = dronePos_;
        simStep[step_].direction = currentDir_;
        simStep[step_].state = droneState_;
        simStep[step_].targetIdx = bestTarget;
        simStep[step_].dropPoint = firePoint;
        simStep[step_].aimPoint = dronePos_ + dir * solver_->getHorizDist();
        simStep[step_].predictedTarget = bestPred_;
        step_++;

        Coord hitDiff = simStep[step_ - 1].aimPoint - simStep[step_ - 1].predictedTarget;
        if (hitDiff.x * hitDiff.x + hitDiff.y * hitDiff.y <= config_.hitRadius * config_.hitRadius)
        {
            targetHit_ = true;
            return simStep[step_ - 1];
        }

        prevBestTarget_ = bestTarget;
        currentTime_ += config_.simTimeStep;
        return std::nullopt;
}

void MissionProcessor::saveResults(const char* path)
{
    json out;
    out["totalSteps"] = step_;
    out["steps"] = json::array();
    
    for ( int i = 0; i < step_; i++) {
        json step;
        step["position"]        = {{"x", simStep[i].pos.x}, {"y", simStep[i].pos.y}};
        step["direction"]       = simStep[i].direction;
        step["state"]           = simStep[i].state;
        step["targetIndex"]     = simStep[i].targetIdx;
        step["dropPoint"]       = {{"x", simStep[i].dropPoint->x},
                                {"y", simStep[i].dropPoint->y}};
        step["aimPoint"]        = {{"x", simStep[i].aimPoint.x},
                                {"y", simStep[i].aimPoint.y}};
        step["predictedTarget"] = {{"x", simStep[i].predictedTarget.x},
                                {"y", simStep[i].predictedTarget.y}};
        out["steps"].push_back(step);
    }
    std::ofstream fout(path + std::string("/simulation.json"));
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
}

float length(Coord delta)
{
    return sqrtf((delta.x) * (delta.x) + (delta.y) * (delta.y));
}
