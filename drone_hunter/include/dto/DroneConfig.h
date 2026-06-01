#pragma once
#include "dto/Coord.h"

struct DroneConfig
{
    Coord startPos;      // початкова позиція (x, y)
    float altitude;      // висота
    float initialDir;    // початковий напрямок (рад)
    float attackSpeed;   // швидкість атаки (м/с)
    float accelPath;     // шлях розгону (м)
    char ammoName[32];   // обрані боєприпаси
    float arrayTimeStep; // крок часу масиву цілей
    float simTimeStep;   // крок симуляції
    float hitRadius;     // радіус влучення
    float angularSpeed;  // кутова швидкість (рад/с)
    float turnThreshold; // поріг повороту (рад)
};