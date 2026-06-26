#include "engine/DronePhysics.h"
#include <algorithm>
#include <chrono>
#include <cmath>

DronePhysics::DronePhysics(Coord startPos, float initialDirection, float attackSpeed,
                            float accelPath, float physicsTimeStep, float timeScale)
    : attackSpeed_(attackSpeed),
      accel_(attackSpeed * attackSpeed / (2.f * accelPath)),
      physicsTimeStep_(physicsTimeStep),
      timeScale_(timeScale),
      pos_(startPos),
      direction_(initialDirection) {
    thread_ = std::thread(&DronePhysics::run, this);
}

DronePhysics::~DronePhysics() {
    stop();
}

void DronePhysics::integrate(float dt) {
    std::lock_guard<std::mutex> lk(mtx_);
    switch (currentCmd_.state) {
        case DroneState::Stopped:
            break;
        case DroneState::Turning:
            direction_ += currentCmd_.angleSpeed * dt;
            break;
        case DroneState::Accelerating:
            speed_ = std::min(speed_ + accel_ * dt, attackSpeed_);
            pos_.x += speed_ * std::cos(direction_) * dt;
            pos_.y += speed_ * std::sin(direction_) * dt;
            break;
        case DroneState::Moving:
            speed_ = attackSpeed_;
            pos_.x += speed_ * std::cos(direction_) * dt;
            pos_.y += speed_ * std::sin(direction_) * dt;
            break;
        case DroneState::Decelerating:
            speed_ = std::max(speed_ - accel_ * dt, 0.f);
            pos_.x += speed_ * std::cos(direction_) * dt;
            pos_.y += speed_ * std::sin(direction_) * dt;
            break;
    }
    simTime_ += dt;
}

void DronePhysics::run() {
    ready_.store(true);
    while (!started_.load() && !stopFlag_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    while (!stopFlag_.load()) {
        while (auto next = queue_.tryPop()) {
            currentCmd_ = *next; // лишаємо лише найновішу команду
        }
        integrate(physicsTimeStep_);
        std::this_thread::sleep_for(std::chrono::duration<float>(physicsTimeStep_ / timeScale_));
    }
}

DroneTelemetry DronePhysics::getTelemetry() const {
    std::lock_guard<std::mutex> lk(mtx_);
    DroneTelemetry t;
    t.pos = pos_;
    t.speed = {speed_ * std::cos(direction_), speed_ * std::sin(direction_)};
    t.direction = direction_;
    t.timeSecSinceStart = simTime_;
    return t;
}

void DronePhysics::stop() {
    stopFlag_.store(true);
    if (thread_.joinable()) thread_.join();
}
