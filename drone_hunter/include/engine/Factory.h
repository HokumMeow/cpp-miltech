#pragma once
#include <string>
class IBallisticSolver;
class ITargetProvider;
class IConfigLoader;

enum class SolverType   { ANALYTICAL };
enum class ProviderType { JSON };
enum class ConfigLoaderType   { FILE };

IBallisticSolver* createSolver(SolverType type);
ITargetProvider*  createProvider(ProviderType type, const std::string& path, float arrayTimeStep);
IConfigLoader*    createConfigLoader(ConfigLoaderType type, const std::string& path);