#pragma once
#include <optional>
#include "dto/AmmoParams.h"
#include "dto/Coord.h"


class IBallisticSolver {
public:
    virtual std::optional<Coord> solve(Coord dronePos, Coord targetPos, 
                            float speed, float alt, 
                            const AmmoParams& ammo) = 0;
    virtual bool precompute(float speed, float alt, const AmmoParams& ammo) = 0;
    virtual float getBallisticTime() const = 0;
    virtual float getHorizDist() const = 0;
    virtual ~IBallisticSolver() {}
};
