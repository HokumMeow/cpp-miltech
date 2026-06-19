#include "engine/Factory.h"
#include "engine/TargetProvider.h"
#include "engine/FileConfigLoader.h"
#include "engine/AnalyticalSolver.h"


ITargetProvider* createProvider(
    ProviderType type, const char* path, float arrayTimeStep) {
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

IConfigLoader* createConfigLoader(ConfigLoaderType type, const char* path) {
    switch (type) {
        case ConfigLoaderType::FILE: return new FileConfigLoader(path);
        default:                     return nullptr;
    }
}