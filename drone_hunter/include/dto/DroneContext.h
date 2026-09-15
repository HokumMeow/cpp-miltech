#pragma once
#include "dto/Coord.h"
#include "dto/DroneConfig.h"

struct DroneContext {
    Coord pos;
    float speed = 0.f;
    float direction = 0.f;
    float angleDiff = 0.f;
    float angleToTarget = 0.f;
    float accel = 0.f;
    const DroneConfig* cfg = nullptr;
};
