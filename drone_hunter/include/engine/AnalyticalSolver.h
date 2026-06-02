#pragma once
#include "interfaces/IBallisticSolver.h"

class AnalyticalSolver : public IBallisticSolver {
public:
    std::optional<Coord> solve(Coord dronePos, Coord targetPos,
                                   float speed, float alt,
                                   const AmmoParams& ammo) override;
    bool precompute(float speed, float alt, const AmmoParams& ammo) override;
    float getBallisticTime() const override;
    float getHorizDist() const override;

private:
    float t_ballist_;
    float h_ballist_;
};