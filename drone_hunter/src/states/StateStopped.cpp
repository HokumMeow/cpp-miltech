#include <cmath>

#include "states/StateStopped.h"
#include "states/StateAccelerating.h"
#include "states/StateTurning.h"

std::unique_ptr<IDroneState> StateStopped::execute(DroneContext& ctx) {
    if (std::fabs(ctx.angleDiff) < ctx.cfg->turnThreshold) {
        return std::make_unique<StateAccelerating>();
    }
    return std::make_unique<StateTurning>();
}

DroneCommand StateStopped::command(const DroneContext&) const {
    return {DroneState::Stopped, 0.f};
}

float StateStopped::timeToStop(const DroneContext&) const {
    return 0.f;
}

const char* StateStopped::name() const {
    return "Stopped";
}

std::unique_ptr<IDroneState> StateStopped::interrupt(const DroneContext& ctx) const {
    if (std::fabs(ctx.angleDiff) > ctx.cfg->turnThreshold) {
        return std::make_unique<StateTurning>();
    }
    return nullptr;
}
