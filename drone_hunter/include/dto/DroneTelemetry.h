#pragma once
#include "dto/Coord.h"

struct DroneTelemetry {
    Coord pos;
    Coord speed;            // вектор швидкості (для логування/виходу)
    float direction = 0.f;  // поточний напрямок (рад) — потрібен окремо,
                             // бо під час Turning speed == (0,0) і напрямок
                             // не відновити з вектора швидкості.
    float timeSecSinceStart = 0.f;
};
