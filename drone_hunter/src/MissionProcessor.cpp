#include "engine/MissionProcessor.h"


float length(Coord delta);

std::optional<SimStep> MissionProcessor::step() {

        float D = 0.f;
        int bestTarget = -1;
        float minTime = 1e9f;
        Coord targInterp;
        Coord predicted;

        targets_->update(currentTime_);

        for (int i = 0; i < targetCount_; ++i) {
            target_ = targets_->getTarget(i);

            interpolate(currentTime_, config_.arrayTimeStep, i, timeSteps_, targets_, targInterp);

            Coord delta = targInterp - dronePos_;

            D = length(delta);
            float totalTime = (D - solver_->getHorizDist()) / config_.attackSpeed + solver_->getBallisticTime();

             for (int k = 0; k < 3; k++)
            {
                interpolate(currentTime_ + totalTime, config_.arrayTimeStep, i, timeSteps_, targets_, predicted);
                delta = predicted - dronePos_;
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
                bestPred_ = predicted;
            }            
        }
        
        //DEBUG("  target=" << bestTarget << " state=" << droneState);

        Coord delta = bestPred_ - dronePos_;

        std::optional<Coord> firePoint = solver_->solve(dronePos_, target_.pos, config_.attackSpeed, config_.altitude, ammo_);

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

        //DEBUG("Step " << step << " pos=(" << dronePos_.x << "," << dronePos_.y << ")");

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
            return simStep[step_ - 1];
        }

        prevBestTarget_ = bestTarget;
        currentTime_ += config_.simTimeStep;
        return std::nullopt;
}

void MissionProcessor::init() {
    
    targetCount_ = targets_->getTargetCount();
    timeSteps_ = targets_->getTimeSteps();
    config_ = loader_->getConfig();
    ammo_        = loader_->getAmmoParams();
    dronePos_    = config_.startPos;
    currentDir_ = config_.initialDir;
    accel_ = config_.attackSpeed * config_.attackSpeed / (2.f * config_.accelPath);
    solver_->precompute(config_.attackSpeed, config_.altitude, ammo_);
    
}
 
float length(Coord delta)
{
    return sqrtf((delta.x) * (delta.x) + (delta.y) * (delta.y));
}

void interpolate(float t, float arrayTimeStep, int targetIndex, int timeSteps , Coord** targets, Coord &output)
{
    int idx = (int)floorf(t / arrayTimeStep) % timeSteps;
    int next = (idx + 1) % timeSteps;
    float frac = (t - idx * arrayTimeStep) / arrayTimeStep;
    output.x = targets[targetIndex][idx].x + (targets[targetIndex][next].x - targets[targetIndex][idx].x) * frac;
    output.y = targets[targetIndex][idx].y + (targets[targetIndex][next].y - targets[targetIndex][idx].y) * frac;
}
