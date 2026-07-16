#include <iostream>
#include <cstring>
#include <cmath>
#include <memory>
#include <span>
#include <thread>
#include <chrono>

#include "json.hpp"

#include "interfaces/IBallisticSolver.h"
#include "interfaces/ITargetProvider.h"
#include "interfaces/IDronePhysics.h"
#include "interfaces/IConfigLoader.h"
#include "engine/MissionProcessor.h"
#include "engine/Factory.h"
#include "Log.h"

using namespace std;
using json = nlohmann::json;

int main(int argc, char* argv[]) {

    std::string path;
    SolverType solverType = SolverType::TABLE;

    const auto kArgs = std::span<char*>(argv, static_cast<std::size_t>(argc));
    if (kArgs.size() < 2) {
        path = "./data";
        LOG("using default data path: " << path << "\n");
        LOG("usage: drone_hunter <data_path> [analytical|table]\n");
    } else {
        path = kArgs[1];
    }

    if (kArgs.size() >= 3) {
        const std::string solverArg = kArgs[2];
        if (solverArg == "analytical") {
            solverType = SolverType::ANALYTICAL;
        } else if (solverArg == "table") {
            solverType = SolverType::TABLE;
        } else {
            std::cerr << "Unknown solver type: " << solverArg << " (expected 'analytical' or 'table')" << std::endl;
            return 1;
        }
    }

    auto loader = createConfigLoader(ConfigLoaderType::FILE, path);
    if (!loader) {
        std::cerr << "Failed to create config loader" << std::endl;
        return 1;
    }
    loader->load();
    const DroneConfig cfg = loader->getConfig();

    auto provider = createProvider(ProviderType::JSON, path, cfg.arrayTimeStep,
                                    cfg.targetTimeStep, cfg.timeScale);
    if (!provider) {
        std::cerr << "Failed to create target provider" << std::endl;
        return 1;
    }

    auto physics = createPhysics(PhysicsType::SIMULATED, cfg.startPos, cfg.initialDir,
                                  cfg.attackSpeed, cfg.accelPath,
                                  cfg.physicsTimeStep, cfg.timeScale);
    if (!physics) {
        std::cerr << "Failed to create drone physics" << std::endl;
        return 1;
    }

    auto solver = createSolver(solverType, path);
    if (!solver) {
        std::cerr << "Failed to create ballistic solver" << std::endl;
        return 1;
    }

    MissionProcessor mission(std::move(solver), std::move(loader), *provider, *physics);

    std::thread providerThread([&provider] { provider->run(); });
    std::thread physicsThread ([&physics]  { physics->run();  });
    std::thread missionThread (&MissionProcessor::run, &mission);

    while (!provider->isThreadReady() || !physics->isThreadReady() || !mission.isThreadReady()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    provider->start();
    physics->start();
    mission.start();

    missionThread.join();

    physics->stop();
    provider->stop();

    physicsThread.join();
    providerThread.join();

    mission.saveResults(path);

    return 0;
}
