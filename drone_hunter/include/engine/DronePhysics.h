#pragma once
#include <atomic>
#include <mutex>
#include <thread>
#include "concurrency/ThreadSafeQueue.h"
#include "dto/Coord.h"
#include "interfaces/IDronePhysics.h"

// "Політний контролер": виконує останню отриману команду, сама інтегрує
// рух і сама клемпує швидкість у межах [0, attackSpeed] — це апаратний
// ліміт контролера, а не рішення місії.
class DronePhysics : public IDronePhysics {
public:
    DronePhysics(Coord startPos, float initialDirection, float attackSpeed,
                 float accelPath, float physicsTimeStep, float timeScale);
    ~DronePhysics() override;

    void sendCommand(const DroneCommand& cmd) override { queue_.push(cmd); }
    DroneTelemetry getTelemetry() const override;

    bool isThreadReady() const override { return ready_.load(); }
    void start() override { started_.store(true); }
    void stop() override;

private:
    void run();
    void integrate(float dt);

    // Паспортні дані контролера
    float attackSpeed_;
    float accel_;
    float physicsTimeStep_;
    float timeScale_;

    // Стан дрона — власність фізики, ніхто інший його не мутує
    mutable std::mutex mtx_;
    Coord pos_;
    float speed_ = 0.f;
    float direction_;
    float simTime_ = 0.f;
    DroneCommand currentCmd_;

    ThreadSafeQueue<DroneCommand> queue_;

    std::atomic<bool> ready_{false};
    std::atomic<bool> started_{false};
    std::atomic<bool> stopFlag_{false};
    std::thread thread_;
};
