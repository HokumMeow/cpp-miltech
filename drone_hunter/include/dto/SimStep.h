#pragma once
#include "dto/Coord.h"
#include <optional>

struct SimStep
{
    Coord pos;             // позиція дрона
    float direction;       // напрямок (рад)
    int state;             // стан автомата (0-4)
    int targetIdx;         // індекс поточної цілі
    std::optional<Coord> dropPoint;       // точка скиду (куди летить дрон)
    Coord aimPoint;        // куди впаде бомба (якщо скинути зараз)
    Coord predictedTarget; // прогнозована позиція цілі
};