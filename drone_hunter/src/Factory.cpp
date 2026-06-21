#include "engine/Factory.h"
#include <memory>
#include <string>
#include "providers/TargetProvider.h"
#include "config/FileConfigLoader.h"
#include "solvers/AnalyticalSolver.h"


std::unique_ptr<ITargetProvider> createProvider(
    ProviderType type, const std::string& path, float arrayTimeStep) {
    switch (type) {
    case ProviderType::JSON:
        return std::make_unique<JsonTargetProvider>(path, arrayTimeStep);
    default: return nullptr;
    }
}

std::unique_ptr<IBallisticSolver> createSolver(SolverType type) {
    switch (type) {
        case SolverType::ANALYTICAL: return std::make_unique<AnalyticalSolver>();
        default:                     return nullptr;
    }
}

std::unique_ptr<IConfigLoader> createConfigLoader(ConfigLoaderType type, const std::string& path) {
    switch (type) {
        case ConfigLoaderType::FILE: return std::make_unique<FileConfigLoader>(path);
        default:                     return nullptr;
    }
}