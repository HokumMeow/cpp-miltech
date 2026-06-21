#pragma once
#include <string>
#include <memory>
class IBallisticSolver;
class ITargetProvider;
class IConfigLoader;

enum class SolverType   { ANALYTICAL };
enum class ProviderType { JSON };
enum class ConfigLoaderType   { FILE };

std::unique_ptr<IBallisticSolver> createSolver(SolverType type);
std::unique_ptr<ITargetProvider>  createProvider(ProviderType type, const std::string& path, float arrayTimeStep);
std::unique_ptr<IConfigLoader>    createConfigLoader(ConfigLoaderType type, const std::string& path);