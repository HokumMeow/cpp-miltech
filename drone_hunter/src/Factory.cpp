#include "engine/Factory.h"
#include <string>
#include "providers/TargetProvider.h"
#include "config/FileConfigLoader.h"
#include "solvers/AnalyticalSolver.h"


ITargetProvider* createProvider(
    ProviderType type, const std::string& path, float arrayTimeStep) {
    switch (type) {
    case ProviderType::JSON:
        return new JsonTargetProvider(path, arrayTimeStep);
    default: return nullptr;
    }
}

IBallisticSolver* createSolver(SolverType type) {
    switch (type) {
        case SolverType::ANALYTICAL: return new AnalyticalSolver();
        default:                     return nullptr;
    }
}

IConfigLoader* createConfigLoader(ConfigLoaderType type, const std::string& path) {
    switch (type) {
        case ConfigLoaderType::FILE: return new FileConfigLoader(path);
        default:                     return nullptr;
    }
}