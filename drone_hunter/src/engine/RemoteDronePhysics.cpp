#include "engine/RemoteDronePhysics.h"
#include <algorithm>
#include <chrono>
#include <thread>

namespace {

// Модуль керування дроном (окремо від MissionProcessor, як вимагає ДЗ11):
// перетворює рішення місії (DroneCommand зі стейт-машини) на нормовані
// accel/turnRate для PKT_CONTROL. maxAngularSpeed — droneCfg.angularSpeed,
// той самий ліміт, яким користується локальна стейт-машина (StateTurning).
dlink::Control toControl(const DroneCommand& cmd, float maxAngularSpeed) {
    dlink::Control c{0.f, 0.f};

    switch (cmd.state) {
        case DroneState::Accelerating: c.accel = 1.f; break;
        case DroneState::Decelerating: c.accel = -1.f; break;
        case DroneState::Moving:
        case DroneState::Turning:
        case DroneState::Stopped:
        default: c.accel = 0.f; break;
    }

    if (maxAngularSpeed > 0.f) {
        c.turnRate = std::clamp(cmd.angleSpeed / maxAngularSpeed, -1.f, 1.f);
    }

    return c;
}

}  // namespace

DroneTelemetry RemoteDronePhysics::getTelemetry() const {
    dlink::Telemetry t = link_.getTelemetry();
    DroneTelemetry out;
    out.pos = {t.x, t.y};
    out.speed = {t.vx, t.vy};
    out.direction = t.dir;
    out.timeSecSinceStart = static_cast<float>(t.t_ms) / 1000.f;
    return out;
}

void RemoteDronePhysics::sendCommand(const DroneCommand& cmd) {
    dlink::DroneCfg droneCfg = link_.getDroneCfg();
    dlink::Control c = toControl(cmd, droneCfg.angularSpeed);
    link_.sendControl(c.accel, c.turnRate);
}

void RemoteDronePhysics::run() {
    ready_.store(true);
    while (!started_.load() && !stopFlag_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    // Читання UART і оновлення телеметрії веде власний потік UartLink;
    // тут додаткової роботи не потрібно, потік лишень сигналізує готовність.
}
