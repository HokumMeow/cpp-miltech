#include "lis3dsh.hpp"

namespace hw18 {

namespace {

// адреси регістрів LIS3DSH з датащиту
constexpr std::uint8_t kRegWhoAmI = 0x0F;
constexpr std::uint8_t kRegCtrlReg4 = 0x20;
constexpr std::uint8_t kRegCtrlReg5 = 0x24;
constexpr std::uint8_t kRegOutXL = 0x28;

constexpr std::uint8_t kWhoAmIExpected = 0x3F;

// ODR=100 Гц (0110), BDU=0, ZEN=YEN=XEN=1  0110 0111.
constexpr std::uint8_t kCtrlReg4Value = 0x67;

constexpr std::uint8_t kCtrlReg5Value = 0x00;

constexpr float kSensitivityGPerLsb = 0.00006F;  // +-2g, датащит LIS3DSH

// біти команди SPI: старший біт - READ, наступний - множинне читання
constexpr std::uint8_t kSpiReadBit = 0x80;
constexpr std::uint8_t kSpiMultiReadBit = 0x40;

}  // namespace

void Lis3dsh::select() noexcept { HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_RESET); }

void Lis3dsh::deselect() noexcept { HAL_GPIO_WritePin(cs_port_, cs_pin_, GPIO_PIN_SET); }

std::uint8_t Lis3dsh::read_reg(std::uint8_t addr) noexcept {
  std::uint8_t tx[2] = {static_cast<std::uint8_t>(kSpiReadBit | addr), 0x00};
  std::uint8_t rx[2] = {0, 0};
  select();
  HAL_SPI_TransmitReceive(spi_, tx, rx, sizeof(tx), 100);
  deselect();
  return rx[1];
}

void Lis3dsh::write_reg(std::uint8_t addr, std::uint8_t value) noexcept {
  std::uint8_t tx[2] = {addr, value};  // старший біт 0 = запис
  std::uint8_t rx[2];
  select();
  HAL_SPI_TransmitReceive(spi_, tx, rx, sizeof(tx), 100);
  deselect();
}

std::uint8_t Lis3dsh::who_am_i() { return read_reg(kRegWhoAmI); }

bool Lis3dsh::init() {
  deselect();
  if (who_am_i() != kWhoAmIExpected) return false;
  write_reg(kRegCtrlReg5, kCtrlReg5Value);
  write_reg(kRegCtrlReg4, kCtrlReg4Value);
  return true;
}

Lis3dsh::Reading Lis3dsh::read() {
  std::uint8_t tx[7] = {static_cast<std::uint8_t>(kSpiReadBit | kSpiMultiReadBit | kRegOutXL)};
  std::uint8_t rx[7] = {};

  select();
  HAL_SPI_TransmitReceive(spi_, tx, rx, sizeof(tx), 100);
  deselect();

  auto axis = [&](std::size_t low_index) -> std::int16_t {
    std::uint16_t raw = static_cast<std::uint16_t>(rx[low_index]) |
                         static_cast<std::uint16_t>(rx[low_index + 1]) << 8;
    return static_cast<std::int16_t>(raw);
  };

  Reading out;
  out.ax_g = static_cast<float>(axis(1)) * kSensitivityGPerLsb;
  out.ay_g = static_cast<float>(axis(3)) * kSensitivityGPerLsb;
  out.az_g = static_cast<float>(axis(5)) * kSensitivityGPerLsb;
  return out;
}

}  // namespace hw18
