#pragma once
#include "IDroneState.h"
#include "dto/DroneContext.h"

class StateDecelerating : public IDroneState {
public:
    std::unique_ptr<IDroneState> execute(DroneContext& ctx) override;
    float timeToStop(const DroneContext& ctx) const override;
    const char* name() const override;
};