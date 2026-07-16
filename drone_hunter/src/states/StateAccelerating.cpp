#include <cmath>

#include "states/StateAccelerating.h"
#include "states/StateMoving.h"

std::unique_ptr<IDroneState> StateAccelerating::execute(DroneContext& ctx) {
    if (ctx.speed >= ctx.cfg->attackSpeed) {
        return std::make_unique<StateMoving>();
    }
    return nullptr;
}

DroneCommand StateAccelerating::command(const DroneContext&) const {
    return {DroneState::Accelerating, 0.f};
}

float StateAccelerating::timeToStop(const DroneContext& ctx) const {
    return ctx.speed / ctx.accel;
}

const char* StateAccelerating::name() const {
    return "Accelerating";
}
