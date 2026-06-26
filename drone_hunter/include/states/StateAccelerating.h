#pragma once
#include "IDroneState.h"
#include "dto/DroneContext.h"

class StateAccelerating : public IDroneState {
public:
    std::unique_ptr<IDroneState> execute(DroneContext& ctx) override;
    DroneCommand command(const DroneContext& ctx) const override;
    float timeToStop(const DroneContext& ctx) const override;
    const char* name() const override;
}; 