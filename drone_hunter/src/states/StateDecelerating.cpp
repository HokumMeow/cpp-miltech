#include <cmath>

#include "states/StateDecelerating.h"
#include "states/StateTurning.h"

std::unique_ptr<IDroneState> StateDecelerating::execute(DroneContext& ctx) {
    ctx.speed -= ctx.accel * ctx.cfg->simTimeStep;

    std::unique_ptr<IDroneState> next;
    if (ctx.speed <= 0.f) {
        ctx.speed = 0.f;
        next = std::make_unique<StateTurning>();
    }

    ctx.pos.x += ctx.speed * std::cos(ctx.direction) * ctx.cfg->simTimeStep;
    ctx.pos.y += ctx.speed * std::sin(ctx.direction) * ctx.cfg->simTimeStep;
    return next;
}

float StateDecelerating::timeToStop(const DroneContext& ctx) const {
    return ctx.speed / ctx.accel;
}

const char* StateDecelerating::name() const {
    return "Decelerating";
}
