#pragma once
#include "dto/Coord.h"
#include "dto/DroneConfig.h"

// Спільні дані, з якими працюють класи станів дрона (states/).
// Власник — MissionProcessor, передається в IDroneState::execute()/timeToStop()
// за посиланням.
struct DroneContext {
    Coord pos;
    float speed = 0.f;
    float direction = 0.f;      // поточний напрямок руху (рад)
    float angleDiff = 0.f;      // direction -> angleToTarget, нормалізований [-PI, PI]
    float angleToTarget = 0.f;  // куди дрону зрештою треба дивитись цього кроку
    float accel = 0.f;
    const DroneConfig* cfg = nullptr;
};
