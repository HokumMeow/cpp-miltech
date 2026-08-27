#pragma once
#include <cmath>

struct Coord
{
    float x = 0.f;
    float y = 0.f;

    // Додавання координат
	Coord operator+(const Coord& other) const {
    	Coord result;
        result.x = x + other.x;
        result.y = y + other.y;
        return result;
	}
 
	// Віднімання координат
	Coord operator-(const Coord& other) const {
    	Coord result;
        result.x = x - other.x;
        result.y = y - other.y;
        return result;
	}
 
	// Множення на скаляр
	Coord operator*(float s) const {
    	Coord result;
        result.x = x * s;
        result.y = y * s;
        return result;
	}

    // Ділення на скаляр
    Coord operator/(float s) const {
        Coord result;
        result.x = x / s;
        result.y = y / s;
        return result;
    }

    // порівняння координат
    bool operator==(const Coord& other) const {
        constexpr float eps = 1e-5f;
        float dx = x - other.x;
        float dy = y - other.y;
        if (dx < 0.f) dx = -dx;
        if (dy < 0.f) dy = -dy;
        return dx < eps && dy < eps;
    }

    // одиничний вектор того ж напрямку
    Coord normalize() const {
        return *this / hypotf(x, y);
    }
};