#include <cmath>

#include "states/StateMoving.h"
#include "states/StateDecelerating.h"

std::unique_ptr<IDroneState> StateMoving::execute(DroneContext& ctx) {
    if (std::fabs(ctx.angleDiff) > ctx.cfg->turnThreshold) {
        return std::make_unique<StateDecelerating>();
    }

    if (std::fabs(ctx.angleDiff) < ctx.cfg->turnThreshold) {
        ctx.direction = ctx.angleToTarget;
    }
    ctx.pos.x += ctx.speed * std::cos(ctx.direction) * ctx.cfg->simTimeStep;
    ctx.pos.y += ctx.speed * std::sin(ctx.direction) * ctx.cfg->simTimeStep;
    return nullptr;
}

float StateMoving::timeToStop(const DroneContext& ctx) const {
    return ctx.cfg->attackSpeed / ctx.accel;
}

const char* StateMoving::name() const {
    return "Moving";
}

std::unique_ptr<IDroneState> StateMoving::interrupt(const DroneContext& ctx) const {
    if (std::fabs(ctx.angleDiff) > ctx.cfg->turnThreshold) {
        return std::make_unique<StateDecelerating>();
    }
    return nullptr;
}
