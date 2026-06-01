#include "engine/MissionProcessor.h"

std::optional<DropPoint> MissionProcessor::step() {
        target_ = targets_->getTarget(currentIdx_++);
        return solver_->solve(dronePos_, target_.pos, attackSpeed_, altitude_, ammo_);
        
}

void MissionProcessor::init() {
    DroneConfig cfg = loader_->getConfig();
    dronePos_    = cfg.startPos;
    altitude_    = cfg.altitude;
    attackSpeed_ = cfg.attackSpeed;
    ammo_        = loader_->getAmmoParams();
    targetCount_ = targets_->getTargetCount();
}
 
