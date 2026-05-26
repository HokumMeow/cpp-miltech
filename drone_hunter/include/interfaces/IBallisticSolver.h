#pragma once
#include "dto/AmmoParams.h"

class IBallisticSolver {
public:
    virtual BallisticTrajectory solve(float speed, float alt, const AmmoParams& ammo
    ) = 0;
    virtual ~IBallisticSolver() {}
};
