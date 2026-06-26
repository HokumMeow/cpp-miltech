#pragma once

// Спільний контракт для компонентів, що живуть у власному потоці
// (ThreadSafeTargetProvider, DronePhysics). Дозволяє main() однаково
// чекати готовності й зупиняти їх, не знаючи конкретного типу.
class IThreadedComponent {
public:
    virtual bool isThreadReady() const = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual ~IThreadedComponent() = default;
};
