#pragma once
#include <string>
#include <memory>
#include "dto/Coord.h"

class IBallisticSolver;
class ITargetProvider;
class IConfigLoader;
class IDronePhysics;
class UartLink;
class GpioLink;

enum class SolverType       { ANALYTICAL, TABLE };
enum class ProviderType     { JSON, UART };
enum class ConfigLoaderType { FILE, UART };
enum class PhysicsType      { SIMULATED, REMOTE };

std::unique_ptr<IBallisticSolver> createSolver(SolverType type, const std::string& path);

// uart потрібен лише для ProviderType::UART; для JSON лишається nullptr.
std::unique_ptr<ITargetProvider>  createProvider(ProviderType type, const std::string& path,
                                                  float arrayTimeStep, float targetTimeStep, float timeScale,
                                                  UartLink* uart = nullptr);

// uart потрібен лише для ConfigLoaderType::UART; для FILE лишається nullptr.
std::unique_ptr<IConfigLoader>    createConfigLoader(ConfigLoaderType type, const std::string& path,
                                                      UartLink* uart = nullptr);

// uart/gpio потрібні лише для PhysicsType::REMOTE; для SIMULATED лишаються nullptr.
std::unique_ptr<IDronePhysics>    createPhysics(PhysicsType type, Coord startPos, float initialDirection,
                                                 float attackSpeed, float accelPath,
                                                 float physicsTimeStep, float timeScale,
                                                 UartLink* uart = nullptr, GpioLink* gpio = nullptr);
