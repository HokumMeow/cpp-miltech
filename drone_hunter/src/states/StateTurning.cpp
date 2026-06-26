#include <cmath>

#include "states/StateTurning.h"
#include "states/StateAccelerating.h"

std::unique_ptr<IDroneState> StateTurning::execute(DroneContext& ctx) {
    float turnStep = ctx.cfg->angularSpeed * ctx.cfg->simTimeStep;

    if (std::fabs(ctx.angleDiff) <= turnStep) {
        return std::make_unique<StateAccelerating>();
    }
    return nullptr;
}

DroneCommand StateTurning::command(const DroneContext& ctx) const {
    float maxStep = ctx.cfg->angularSpeed * ctx.cfg->simTimeStep;
    float magnitude = ctx.cfg->angularSpeed;
    if (std::fabs(ctx.angleDiff) < maxStep) {
        // не проскочити повз ціль до наступного перерахунку місії
        magnitude = std::fabs(ctx.angleDiff) / ctx.cfg->simTimeStep;
    }
    float sign = (ctx.angleDiff >= 0.f) ? 1.f : -1.f;
    return {DroneState::Turning, sign * magnitude};
}

float StateTurning::timeToStop(const DroneContext& ctx) const {
    return std::fabs(ctx.angleDiff) / ctx.cfg->angularSpeed;
}

const char* StateTurning::name() const {
    return "Turning";
}
