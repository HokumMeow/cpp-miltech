#pragma once

enum class DroneState {
    Stopped,
    Turning,
    Accelerating,
    Moving,
    Decelerating
};

struct DroneCommand {
    DroneState state = DroneState::Stopped;
    float angleSpeed = 0.f;
};
