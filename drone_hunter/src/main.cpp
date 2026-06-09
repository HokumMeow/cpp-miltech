#include <iostream>
#include <cstring>
#include <cmath>

#include "json.hpp"

#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "engine/MissionProcessor.h"
#include "engine/Factory.h"
#include "Log.h"

using namespace std;
using json = nlohmann::json;

int main(int argc, char* argv[]) {
    
    std::string path;

    const auto kArgs = std::span<char*>(argv, static_cast<std::size_t>(argc));
    if (kArgs.size() < 2) {
        path = "./data";
        LOG("using default data path: " << path << "\n");
        LOG("usage custom data path: drone_hunter <data_path>\n");
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