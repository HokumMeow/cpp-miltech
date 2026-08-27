#pragma once
#include "dto/DroneCommand.h"
#include "dto/DroneTelemetry.h"
#include "interfaces/IThreadedComponent.h"

class IDronePhysics : public IThreadedComponent {
public:
    virtual void sendCommand(const DroneCommand& cmd) = 0;
    virtual DroneTelemetry getTelemetry() const = 0;
    virtual void drop() {};
    virtual ~IDronePhysics() override {}
};
