#include <cmath>

#include "states/StateAccelerating.h"
#include "states/StateMoving.h"

std::unique_ptr<IDroneState> StateAccelerating::execute(DroneContext& ctx) {
    ctx.speed += ctx.accel * ctx.cfg->simTimeStep;

    std::unique_ptr<IDroneState> next;
    if (ctx.speed >= ctx.cfg->attackSpeed) {
        ctx.speed = ctx.cfg->attackSpeed;
        next = std::make_unique<StateMoving>();
    }

    ctx.pos.x += ctx.speed * std::cos(ctx.direction) * ctx.cfg->simTimeStep;
    ctx.pos.y += ctx.speed * std::sin(ctx.direction) * ctx.cfg->simTimeStep;
    return next;
}

float StateAccelerating::timeToStop(const DroneContext& ctx) const {
    return ctx.speed / ctx.accel;
}

const char* StateAccelerating::name() const {
    return "Accelerating";
}
