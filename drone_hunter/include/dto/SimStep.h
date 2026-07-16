#pragma once
#include "dto/Coord.h"
#include <optional>
#include <string>

struct SimStep
{
    Coord pos;             // позиція дрона
    float direction;       // напрямок (рад)
    std::string state;     // назва стану автомата
    int targetIdx;         // індекс поточної цілі
    std::optional<Coord> dropPoint;       // точка скиду (куди летить дрон)
    Coord aimPoint;        // куди впаде бомба (якщо скинути зараз)
    Coord predictedTarget; // прогнозована позиція цілі
    float timeSecSinceStart; // момент фізики, на який знято цей знімок
};