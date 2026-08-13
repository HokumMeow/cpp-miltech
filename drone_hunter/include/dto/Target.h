#pragma once
#include <chrono>
#include "Coord.h"

struct Target {
    Coord pos;
    Coord velocity;
};

struct TargetHistory {
    Coord pos;
    std::chrono::steady_clock::time_point lastUpdate;
    bool seen = false;
};