#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

#include "json.hpp"

#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "core/MissionProcessor.h"
#include "core/Factory.h"


using namespace std;
using json = nlohmann::json;

#define ENABLE_LOG 1
#define ENABLE_DEBUG 0

#if ENABLE_LOG
#define LOG(msg) cout << "[LOG] " << msg << endl
#else
#define LOG(msg)
#endif

#if ENABLE_DEBUG
#define DEBUG(msg) cout << "[DEBUG] " << msg << endl
#else
#define DEBUG(msg)
#endif

int main() {
    
    
    IBallisticSolver* solver = createSolver(SolverType::ANALYTICAL);
    ITargetProvider* targets = createProvider(ProviderType::JSON, "targets.json");
    IConfigLoader*   loader  = createLoader(LoaderType::FILE);

    
    MissionProcessor mission(solver, targets, loader);
    mission.init("mission.cfg");

    while (mission.hasNext()) {
        DropPoint dp = mission.step();
        printf("drop: (%.1f, %.1f)\n", dp.coord.x, dp.coord.y);
    }

    //LOG("Simulation finished in " << step << " steps, time=" << currentTime << "s");

    delete solver;
    delete targets;
    delete loader;

    return 0;
}