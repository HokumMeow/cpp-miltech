#pragma once
#include <memory>
#include "dto/DroneCommand.h"
#include "dto/DroneContext.h"

class IDroneState {
public:
 virtual ~IDroneState() = default;
 virtual std::unique_ptr<IDroneState> execute(DroneContext& ctx) = 0;
 virtual DroneCommand command(const DroneContext& ctx) const = 0;
 virtual float timeToStop(const DroneContext&) const = 0;
 virtual const char* name() const = 0;
 virtual std::unique_ptr<IDroneState> interrupt(const DroneContext&) const { return nullptr; }
};
