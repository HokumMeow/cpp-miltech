#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

#include "json.hpp"

#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "core/MissionProcessor.h"
#include "core/Factory.h"
#include "core/Log.h"

using namespace std;
using json = nlohmann::json;

int main() {
        
    IBallisticSolver* solver = createSolver(SolverType::ANALYTICAL);
    ITargetProvider* targets = createProvider(ProviderType::JSON, "targets.json");
    IConfigLoader*   loader  = createLoader(LoaderType::FILE);
    
    MissionProcessor mission(solver, targets, loader);
    mission.init("mission.cfg");

    while (mission.hasNext()) {
        DropPoint dp = mission.step();
        LOG("drop: (" << dp.coord.x << ", " << dp.coord.y << ")");
    }

    LOG("Simulation finished");

    delete solver;
    delete targets;
    delete loader;

    return 0;
}