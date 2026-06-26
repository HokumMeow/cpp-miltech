#pragma once
#include <string>
#include "dto/Coord.h"

struct DroneConfig
{
    Coord startPos;      // початкова позиція (x, y)
    float altitude;      // висота
    float initialDir;    // початковий напрямок (рад)
    float attackSpeed;   // швидкість атаки (м/с)
    float accelPath;     // шлях розгону (м)
    std::string ammoName; // обрані боєприпаси
    float arrayTimeStep; // крок часу масиву цілей
    float simTimeStep;   // крок симуляції
    float hitRadius;     // радіус влучення
    float angularSpeed;  // кутова швидкість (рад/с)
    float turnThreshold; // поріг повороту (рад)

    float targetTimeStep  = 0.05f; // крок потоку ThreadSafeTargetProvider
    float physicsTimeStep = 0.01f; // крок потоку DronePhysics
    float timeScale       = 10.f;  // прискорення симуляції відносно реального часу
};