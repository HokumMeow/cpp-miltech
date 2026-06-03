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
    
    const char* path = nullptr;

    const auto kArgs = std::span<char*>(argv, static_cast<std::size_t>(argc));
    if (kArgs.size() < 2) {
        path = "./data";
        //std::cerr << "usage: drone_hunter <data_path>\n";
        //return 1;

    } else {
        path = kArgs[1];  
    }

    IConfigLoader*   loader  = createConfigLoader(ConfigLoaderType::FILE, path);
    loader->load();
    float arrayTimeStep = loader->getConfig().arrayTimeStep;

    ITargetProvider* targets = createProvider(ProviderType::JSON, path, arrayTimeStep);
    IBallisticSolver* solver = createSolver(SolverType::ANALYTICAL);
            
    MissionProcessor mission(solver, targets, loader);
    mission.init();

    while (mission.hasNext()) {
        auto result = mission.step();
        if (result.has_value()) {
            LOG("Hit! drop at (" << result->dropPoint->x << ", " << result->dropPoint->y << ")");  
            break;
        }
    }

    LOG("Simulation finished");

    mission.saveResults(path);

    delete solver;
    delete targets;
    delete loader;

    return 0;
}