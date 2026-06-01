#include <iostream>
#include <fstream>
#include <cstring>
#include <cmath>

#include "json.hpp"

#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "engine/MissionProcessor.h"
#include "engine/Factory.h"
#include "engine/Log.h"

using namespace std;
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    
    const auto kArgs = std::span<char*>(argv, static_cast<std::size_t>(argc));
    if (kArgs.size() < 2) {
        std::cerr << "usage: drone_hunter <data_path>\n";
        return 1;
    }

    const char* path = kArgs[1];    

    IConfigLoader*   loader  = createLoader(ConfigLoaderType::FILE, path);
    loader->load();
    float arrayTimeStep = loader->getConfig().arrayTimeStep;

    ITargetProvider* targets = createProvider(ProviderType::JSON, path, arrayTimeStep);
    IBallisticSolver* solver = createSolver(SolverType::ANALYTICAL);
            
    MissionProcessor mission(solver, targets, loader);
    mission.init();

    while (mission.hasNext()) {
        std::optional<DropPoint> dp = mission.step();
        if (dp.has_value()) {
            LOG("drop: (" << dp.value().coord.x << ", " << dp.value().coord.y << ")");
        } else {
            LOG("no solution for current target");
        }
    }

    LOG("Simulation finished");

    delete solver;
    delete targets;
    delete loader;

    return 0;
}