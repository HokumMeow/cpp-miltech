#pragma once

// Режим, який MissionProcessor наказує виконати DronePhysics.
// Дзеркалить імена класів станів (StateStopped/...), але це лише ярлик
// для фізики — вона не знає про стейт-машину місії.
enum class DroneState {
    Stopped,
    Turning,
    Accelerating,
    Moving,
    Decelerating
};

struct DroneCommand {
    DroneState state = DroneState::Stopped;
    float angleSpeed = 0.f; // зі знаком: напрямок повороту цього кроку
};
