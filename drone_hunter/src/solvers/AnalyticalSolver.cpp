#include <optional>
#include <cmath>
#include "solvers/AnalyticalSolver.h"
#include "dto/AmmoParams.h"
#include "dto/Coord.h"

float calc_t(const AmmoParams& a, float attackSpeed, float zd);
float calc_h(const AmmoParams& a, float attackSpeed, float t);
Coord normalize(Coord c);

static constexpr float PI = 3.14159265f;
static constexpr float g  = 9.81f;

std::optional<Coord> AnalyticalSolver::solve(Coord dronePos, Coord targetPos,
                float speed, float alt, const AmmoParams& ammo
)  {
    float t_ballist = calc_t(ammo, speed, alt);
    if (t_ballist == -1.f) return std::nullopt;
    float hDist = calc_h(ammo, speed, t_ballist);
    if (hDist <= 0.f) return std::nullopt;
    Coord firePoint = targetPos - normalize(targetPos - dronePos) * hDist;
    return Coord{firePoint.x, firePoint.y};
}

bool AnalyticalSolver::precompute(float speed, float alt, const AmmoParams& ammo) {
    t_ballist_ = calc_t(ammo, speed, alt);
    if (t_ballist_ == -1.f) return false;
    h_ballist_ = calc_h(ammo, speed, t_ballist_);
    if (h_ballist_ <= 0.f) return false;
    return true;
}

float AnalyticalSolver::getBallisticTime() const {
    return t_ballist_;
}

float AnalyticalSolver::getHorizDist() const {
    return h_ballist_;
}

float calc_t(const AmmoParams& a, float attackSpeed, float zd)
{
    float d = a.drag;
    float m = a.mass;
    float l = a.lift;
    float a_val = d * g * m - 2.f * d * d * l * attackSpeed;
    float b = -3.f * g * m * m + 3.f * d * l * m * attackSpeed;
    float c = 6.f * m * m * zd;
    float p = (-(b * b)) / (3.f * a_val * a_val);
    float q = 2.f * b * b * b / (27.f * a_val * a_val * a_val) + c / a_val;
    double acosArg = 3.f * q / (2.f * p) * sqrtf(-3.f / p);
    if (acosArg < -1.0 || acosArg > 1.0)
    {
        return -1;
    }
    float phi = acosf(acosArg);
    float t = 2.f * sqrtf(-p / 3.f) * cosf((phi + 4.0 * PI) / 3.f) - b / (3.0 * a_val);
    if (t <= 0)
    {
        return -1;
    }
    return t;
}

float calc_h(const AmmoParams& a, float attackSpeed, float t)
{
    float t2 = t * t, t3 = t2 * t, t4 = t3 * t, t5 = t4 * t;
    float d2 = a.drag * a.drag, d3 = d2 * a.drag, d4 = d3 * a.drag;
    float l2 = a.lift * a.lift, l3 = l2 * a.lift, l4 = l3 * a.lift;
    float m2 = a.mass * a.mass, m3 = m2 * a.mass, m4 = m3 * a.mass;
    float result = attackSpeed * t - t2 * a.drag * attackSpeed 
    / (2.f * a.mass) + t3 * (6.f * a.drag * g * a.lift * a.mass - 6.f * d2 * (l2 - 1.f) * attackSpeed) 
    / (36.f * m2) + t4 * (-6.f * d2 * g * a.lift * (1.f + l2 + l4) * a.mass + 3.f * d3 * l2 * (1.f + l2) * attackSpeed + 6.f * d2 * a.drag * l4 * (1.f + l2) * attackSpeed) 
    / (36.f * (1.f + l2) * (1.f + l2) * m3) + t5 * (3.f * d3 * g * l3 * a.mass - 3.f * d4 * l2 * (1.f + l2) * attackSpeed) 
    / (36.f * (1.f + l2) * m4);
    return result;
}

Coord normalize(Coord c) {
    return c / hypot(c.x, c.y);
}

