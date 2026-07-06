#include <cmath>

#include "states/StateTurning.h"
#include "states/StateAccelerating.h"

std::unique_ptr<IDroneState> StateTurning::execute(DroneContext& ctx) {
    float turnStep = ctx.cfg->angularSpeed * ctx.cfg->simTimeStep;

    if (std::fabs(ctx.angleDiff) <= turnStep) {
        ctx.direction = ctx.angleToTarget;
        ctx.angleDiff = 0.f;
        return std::make_unique<StateAccelerating>();
    }

    ctx.direction += (ctx.angleDiff > 0 ? 1.f : -1.f) * turnStep;
    return nullptr;
}

float StateTurning::timeToStop(const DroneContext& ctx) const {
    return std::fabs(ctx.angleDiff) / ctx.cfg->angularSpeed;
}

const char* StateTurning::name() const {
    return "Turning";
}
