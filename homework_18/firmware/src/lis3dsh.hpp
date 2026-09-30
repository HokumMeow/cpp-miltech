// драйвер акселерометра LIS3DSH на платі STM32F4-Discovery
#pragma once

#include <cstdint>

#include "stm32f4xx_hal.h"

namespace hw18 {

class Lis3dsh {
 public:
  struct Reading {
    float ax_g = 0.0F;
    float ay_g = 0.0F;
    float az_g = 0.0F;
  };

  Lis3dsh(SPI_HandleTypeDef* spi, GPIO_TypeDef* cs_port, std::uint16_t cs_pin)
      : spi_(spi), cs_port_(cs_port), cs_pin_(cs_pin) {}

  // читає WHO_AM_I і якщо він збігається з очікуваним 0x3F, включає
  // вимір по X/Y/Z на 100 Гц у діапазоні +-2g. 
  // повертає false, якщо датчик не відповів або відповів не як LIS3DSH
  bool init();

  [[nodiscard]] Reading read();

  [[nodiscard]] std::uint8_t who_am_i();

 private:
  void select() noexcept;
  void deselect() noexcept;
  [[nodiscard]] std::uint8_t read_reg(std::uint8_t addr) noexcept;
  void write_reg(std::uint8_t addr, std::uint8_t value) noexcept;

  SPI_HandleTypeDef* spi_;
  GPIO_TypeDef* cs_port_;
  std::uint16_t cs_pin_;
};

}  // namespace hw18
