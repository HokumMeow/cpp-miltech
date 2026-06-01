#pragma once
#include "interfaces/IBallisticSolver.h"

class AnalyticalSolver : public IBallisticSolver {
public:
    std::optional<DropPoint> solve(Coord dronePos, Coord targetPos,
                                   float speed, float alt,
                                   const AmmoParams& ammo) override;
private:
    float calc_t(const AmmoParams& a, float speed, float alt);
    float calc_h(const AmmoParams& a, float speed, float t);
    Coord normalize(Coord c);
};