#pragma once
#include "dto/DroneCommand.h"
#include "dto/DroneTelemetry.h"
#include "interfaces/IThreadedComponent.h"

// Контракт "політного контролера" дрона: приймає команди, віддає
// телеметрію. Сьогодні єдина реалізація (DronePhysics) сама інтегрує рух
// формулами. Пізніше та сама точка підключення дозволить підставити
// реалізацію, що читає реальну телеметрію (напр. з GPIO/пінів), без змін
// у MissionProcessor.
class IDronePhysics : public IThreadedComponent {
public:
    virtual void sendCommand(const DroneCommand& cmd) = 0;
    virtual DroneTelemetry getTelemetry() const = 0;
    virtual ~IDronePhysics() override {}
};
