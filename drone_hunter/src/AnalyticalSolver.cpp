#include "interfaces/IBallisticSolver.h"
#include "core/Factory.h"
#include "dto/AmmoParams.h"
#include "dto/Coord.h"

class AnalyticalSolver
    : public IBallisticSolver {
public:
    DropPoint solve(Coord drone, Coord tgt,
        float alt, const AmmoParams& a
    ) override {
        float t = sqrt(2.0f * alt / 9.81f);
        float drift = a.dragCoeff * t;
        return {tgt.x - drift, tgt.y};
    }
};

class TableSolver
    : public IBallisticSolver {
    BallisticTable* table;
public:
    TableSolver(BallisticTable* t)
        : table(t) {}
    DropPoint solve(Coord drone, Coord tgt,
        float alt, const AmmoParams& a
    ) override {
        return table->interpolate(
            alt, dist(drone, tgt), a);
    }
};



IBallisticSolver* createSolver(SolverType type) {
    switch (type) {
        case SolverType::ANALYTICAL: return new AnalyticalSolver();
        // case SolverType::TABLE:   return new TableSolver(...);   // коли додаси
        default:                     return nullptr;
    }
}