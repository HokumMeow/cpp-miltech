#pragma once
#include "dto/AmmoParams.h"
#include "dto/DropPoint.h"


class IBallisticSolver {
public:
    virtual DropPoint solve(float speed, float alt, const AmmoParams& ammo
    ) = 0;
    virtual ~IBallisticSolver() {}
};
