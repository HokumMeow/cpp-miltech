#pragma once
#include "dto/Coord.h"

struct DroneTelemetry {
    Coord pos;
    Coord speed;
    float direction = 0.f;
    float timeSecSinceStart = 0.f;
};
