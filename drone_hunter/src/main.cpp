#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

#include "json.hpp"

#include "dto/Target.h"


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

const float g = 9.81f;
const int MAX_STEPS = 10000;
const float PI = 3.14159265f;

int main() {
    
    
    IBallisticSolver* solver = createSolver(SolverType::ANALYTICAL);
    ITargetProvider* targets = createProvider(ProviderType::JSON, "targets.json");
    IConfigLoader*   loader  = createLoader(LoaderType::FILE);

    
    MissionProcessor mission(solver, targets, loader);
    mission.init("mission.cfg");

    while (mission.hasNext()) {
        DropPoint dp = mission.step();
        printf("drop: (%.1f, %.1f)\n", dp.x, dp.y);
    }

    LOG("Simulation finished in " << step << " steps, time=" << currentTime << "s");

    delete solver;
    delete targets;
    delete loader;

    return 0;
}