// періодичний вимір акселерометра, звіт по UART, команди з UART, кнопка
#pragma once

#include <cstdint>

#include "hw18/protocol.hpp"
#include "lis3dsh.hpp"
#include "stm32f4xx_hal.h"

namespace hw18 {

class App {
 public:
  App(SPI_HandleTypeDef* spi, UART_HandleTypeDef* uart, TIM_HandleTypeDef* tim,
      IWDG_HandleTypeDef* iwdg, GPIO_TypeDef* accel_cs_port, std::uint16_t accel_cs_pin,
      GPIO_TypeDef* led_port, std::uint16_t led_pin);

  // ініціалізація датчика по UART
  void begin();

  // тіло головного циклу
  void poll();

  void on_timer_tick() noexcept { tick_pending_ = true; }
  void on_uart_rx_byte(std::uint8_t byte) noexcept { rx_.push_byte(byte); }
  void on_button_edge() noexcept { button_pending_ = true; }

 private:
  void handle_tick();
  void handle_button();
  void handle_uart_commands();
  void apply_period(std::uint32_t period_ms);
  void apply_mode(Mode mode);
  void send_line(const char* text, std::size_t len);

  SPI_HandleTypeDef* spi_;
  UART_HandleTypeDef* uart_;
  TIM_HandleTypeDef* tim_;
  IWDG_HandleTypeDef* iwdg_;
  GPIO_TypeDef* led_port_;
  std::uint16_t led_pin_;

  Lis3dsh accel_;
  LineAssembler rx_;

  volatile bool tick_pending_ = false;
  volatile bool button_pending_ = false;

  std::uint32_t period_ms_ = 500;
  Mode mode_ = Mode::Slow;
  std::uint32_t last_button_ms_ = 0;

  static constexpr std::uint32_t kButtonDebounceMs = 20;
  static constexpr std::uint32_t kFastPeriodMs = 100;
  static constexpr std::uint32_t kSlowPeriodMs = 500;
};

}  // namespace hw18
