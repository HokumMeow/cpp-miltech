#pragma once
#include <memory>
#include "dto/DroneCommand.h"
#include "dto/DroneContext.h"

class IDroneState {
public:
 virtual ~IDroneState() = default;
 // Вирішити, чи час переходити в інший стан (рішення, не виконання).
 // Якщо стан не змінився — повернути nullptr
 // (головний цикл залишить поточний).
 virtual std::unique_ptr<IDroneState> execute(DroneContext& ctx) = 0;
 // Команда, яку треба надіслати DronePhysics, поки активний цей стан.
 virtual DroneCommand command(const DroneContext& ctx) const = 0;
 virtual float timeToStop(const DroneContext&) const = 0;
 virtual const char* name() const = 0;

 // Викликається ДО execute() того ж кадру, коли свіжо обчислений
 // angleDiff виявився завеликим. Дозволяє стану відразу "перескочити"
 // в інший (STOPPED -> TURNING, MOVING -> DECELERATING), щоб execute()
 // цього ж кадру вже виконував логіку нового стану.
 // Більшість станів це не цікавить — звідси non-pure default.
 virtual std::unique_ptr<IDroneState> interrupt(const DroneContext&) const { return nullptr; }
};
