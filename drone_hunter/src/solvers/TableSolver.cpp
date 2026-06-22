#include "solvers/TableSolver.h"
#include <algorithm>
#include <fstream>

namespace {

BallisticTable::Result lerp(const BallisticTable::Result& a, const BallisticTable::Result& b, float t) {
    return { a.t + (b.t - a.t) * t, a.hDist + (b.hDist - a.hDist) * t };
}

struct Interp {
    int lo;     // нижній індекс в осі
    float frac; // коефіцієнт [0..1]
};

Interp findInterp(float val, const std::vector<float>& axis) {
    if (val <= axis.front()) return {0, 0.0f};
    if (val >= axis.back())  return {(int)axis.size() - 2, 1.0f};

    auto it = std::lower_bound(axis.begin(), axis.end(), val);
    int i = (int)(it - axis.begin()) - 1;
    if (i < 0) i = 0;
    float frac = (val - axis[i]) / (axis[i + 1] - axis[i]);
    return {i, frac};
}

} // namespace

bool BallisticTable::load(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    int nZ, nV, nM, nD, nL;
    f >> nZ >> nV >> nM >> nD >> nL;

    axisZ0.resize(nZ); for (auto& v : axisZ0) f >> v;
    axisV0.resize(nV); for (auto& v : axisV0) f >> v;
    axisM.resize(nM);  for (auto& v : axisM)  f >> v;
    axisD.resize(nD);  for (auto& v : axisD)  f >> v;
    axisL.resize(nL);  for (auto& v : axisL)  f >> v;

    size_t total = (size_t)nZ * nV * nM * nD * nL;
    data.resize(total);

    // Порядок: Z0 → V0 → m → d → l (зовнішній → внутрішній)
    for (size_t i = 0; i < total; i++)
        f >> data[i].t >> data[i].hDist;

    return !f.fail();
}

BallisticTable::Result BallisticTable::lookup(float Z0, float V0, float m, float d, float l) const {
    Interp iz = findInterp(Z0, axisZ0);
    Interp iv = findInterp(V0, axisV0);
    Interp im = findInterp(m, axisM);
    Interp id = findInterp(d, axisD);
    Interp il = findInterp(l, axisL);

    // 2^5 = 32 вершини гіперкуба, згортаємо: 32 → 16 → 8 → 4 → 2 → 1
    Result v[16];
    for (int a = 0; a < 2; a++)
        for (int b = 0; b < 2; b++)
            for (int c = 0; c < 2; c++)
                for (int e = 0; e < 2; e++) {
                    const Result& lo = at(iz.lo + a, iv.lo + b, im.lo + c, id.lo + e, il.lo);
                    const Result& hi = at(iz.lo + a, iv.lo + b, im.lo + c, id.lo + e, il.lo + 1);
                    v[a * 8 + b * 4 + c * 2 + e] = lerp(lo, hi, il.frac);
                }

    Result w[8];
    for (int a = 0; a < 2; a++)
        for (int b = 0; b < 2; b++)
            for (int c = 0; c < 2; c++)
                w[a * 4 + b * 2 + c] = lerp(v[a * 8 + b * 4 + c * 2], v[a * 8 + b * 4 + c * 2 + 1], id.frac);

    Result u[4];
    for (int a = 0; a < 2; a++)
        for (int b = 0; b < 2; b++)
            u[a * 2 + b] = lerp(w[a * 4 + b * 2], w[a * 4 + b * 2 + 1], im.frac);

    Result s[2];
    for (int a = 0; a < 2; a++)
        s[a] = lerp(u[a * 2], u[a * 2 + 1], iv.frac);

    return lerp(s[0], s[1], iz.frac);
}

TableSolver::TableSolver(const std::string& tablePath) {
    loaded_ = table_.load(tablePath);
}

std::optional<Coord> TableSolver::solve(Coord dronePos, Coord targetPos,
                                         float speed, float alt, const AmmoParams& ammo) {
    if (!loaded_) return std::nullopt;
    BallisticTable::Result r = table_.lookup(alt, speed, ammo.mass, ammo.drag, ammo.lift);
    if (r.hDist <= 0.f) return std::nullopt;
    Coord firePoint = targetPos - (targetPos - dronePos).normalize() * r.hDist;
    return firePoint;
}

bool TableSolver::precompute(float speed, float alt, const AmmoParams& ammo) {
    if (!loaded_) return false;
    BallisticTable::Result r = table_.lookup(alt, speed, ammo.mass, ammo.drag, ammo.lift);
    if (r.hDist <= 0.f) return false;
    t_ballist_ = r.t;
    h_ballist_ = r.hDist;
    return true;
}

float TableSolver::getBallisticTime() const {
    return t_ballist_;
}

float TableSolver::getHorizDist() const {
    return h_ballist_;
}
