#pragma once
#include <string>
#include <memory>
#include "dto/Coord.h"

class IBallisticSolver;
class ITargetProvider;
class IConfigLoader;
class IDronePhysics;

enum class SolverType       { ANALYTICAL, TABLE };
enum class ProviderType     { JSON };
enum class ConfigLoaderType { FILE };
enum class PhysicsType      { SIMULATED };

std::unique_ptr<IBallisticSolver> createSolver(SolverType type, const std::string& path);
std::unique_ptr<ITargetProvider>  createProvider(ProviderType type, const std::string& path,
                                                  float arrayTimeStep, float targetTimeStep, float timeScale);
std::unique_ptr<IConfigLoader>    createConfigLoader(ConfigLoaderType type, const std::string& path);
std::unique_ptr<IDronePhysics>    createPhysics(PhysicsType type, Coord startPos, float initialDirection,
                                                 float attackSpeed, float accelPath,
                                                 float physicsTimeStep, float timeScale);
