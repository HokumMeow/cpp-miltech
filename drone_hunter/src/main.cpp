#include <iostream>
#include <cstring>
#include <cmath>
#include <memory>
#include <span>

#include "json.hpp"

#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "interfaces/IConfigLoader.h"
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

    auto loader = createConfigLoader(ConfigLoaderType::FILE, path);
    if (!loader) {
        std::cerr << "Failed to create config loader" << std::endl;
        return 1;
    }
    loader->load();
    float arrayTimeStep = loader->getConfig().arrayTimeStep;

    auto targets = createProvider(ProviderType::JSON, path, arrayTimeStep);
    if (!targets) {
        std::cerr << "Failed to create target provider" << std::endl;
        return 1;
    }

    auto solver = createSolver(SolverType::ANALYTICAL, path);
    if (!solver) {
        std::cerr << "Failed to create ballistic solver" << std::endl;
        return 1;
    }

    MissionProcessor mission(std::move(solver), std::move(targets), std::move(loader));
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

    return 0;
}