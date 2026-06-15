#pragma once
class IBallisticSolver;
class ITargetProvider;
class IConfigLoader;

enum class SolverType   { ANALYTICAL };
enum class ProviderType { JSON };
enum class ConfigLoaderType   { FILE };

IBallisticSolver* createSolver(SolverType type);
ITargetProvider*  createProvider(ProviderType type, const char* path, float arrayTimeStep);
IConfigLoader*    createConfigLoader(ConfigLoaderType type, const char* path);