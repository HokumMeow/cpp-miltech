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
};