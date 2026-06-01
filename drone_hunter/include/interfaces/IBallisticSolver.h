#pragma once
#include <optional>
#include "dto/AmmoParams.h"
#include "dto/DropPoint.h"
#include "dto/Coord.h"


class IBallisticSolver {
public:
    virtual std::optional<DropPoint> solve(Coord dronePos, Coord targetPos, 
                            float speed, float alt, 
                            const AmmoParams& ammo) = 0;
    virtual ~IBallisticSolver() {}
};
