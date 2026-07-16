#include <cmath>

#include "states/StateDecelerating.h"
#include "states/StateTurning.h"

std::unique_ptr<IDroneState> StateDecelerating::execute(DroneContext& ctx) {
    if (ctx.speed <= 0.f) {
        return std::make_unique<StateTurning>();
    }
    return nullptr;
}

DroneCommand StateDecelerating::command(const DroneContext&) const {
    return {DroneState::Decelerating, 0.f};
}

float StateDecelerating::timeToStop(const DroneContext& ctx) const {
    return ctx.speed / ctx.accel;
}

const char* StateDecelerating::name() const {
    return "Decelerating";
}
