#include "engine/Factory.h"
#include <memory>
#include <string>
#include "providers/ThreadSafeTargetProvider.h"
#include "engine/DronePhysics.h"
#include "config/FileConfigLoader.h"
#include "solvers/AnalyticalSolver.h"
#include "solvers/TableSolver.h"


std::unique_ptr<ITargetProvider> createProvider(
    ProviderType type, const std::string& path, float arrayTimeStep,
    float targetTimeStep, float timeScale) {
    switch (type) {
    case ProviderType::JSON:
        return std::make_unique<ThreadSafeTargetProvider>(path, arrayTimeStep, targetTimeStep, timeScale);
    default: return nullptr;
    }
}

std::unique_ptr<IBallisticSolver> createSolver(SolverType type, const std::string& path) {
    switch (type) {
        case SolverType::ANALYTICAL: return std::make_unique<AnalyticalSolver>();
        case SolverType::TABLE:      return std::make_unique<TableSolver>(path + "/ballistic_table.txt");
        default:                     return nullptr;
    }
}

std::unique_ptr<IConfigLoader> createConfigLoader(ConfigLoaderType type, const std::string& path) {
    switch (type) {
        case ConfigLoaderType::FILE: return std::make_unique<FileConfigLoader>(path);
        default:                     return nullptr;
    }
}

std::unique_ptr<IDronePhysics> createPhysics(PhysicsType type, Coord startPos, float initialDirection,
                                              float attackSpeed, float accelPath,
                                              float physicsTimeStep, float timeScale) {
    switch (type) {
        case PhysicsType::SIMULATED:
            return std::make_unique<DronePhysics>(startPos, initialDirection, attackSpeed,
                                                   accelPath, physicsTimeStep, timeScale);
        default: return nullptr;
    }
}
