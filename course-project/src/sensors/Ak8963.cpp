#include "sensors/Ak8963.h"

#include <chrono>
#include <thread>

namespace {

// AK8963 молодший байт першим
int16_t combineLittleEndian(uint8_t lo, uint8_t hi) {
    return static_cast<int16_t>((static_cast<uint16_t>(hi) << 8) | lo);
}

constexpr uint8_t kSt1DataReady = 0x01;
constexpr uint8_t kSt2Overflow = 0x08;

}  // namespace

Ak8963::Ak8963(I2cBus& bus) : bus_(bus) {}

uint8_t Ak8963::readWhoAmI() const {
    return bus_.readRegister(ak8963::kRegWhoAmI);
}

void Ak8963::startContinuous() const {
    // для зміну режиму треба power-down
    bus_.writeRegister(ak8963::kRegCntl1, ak8963::kModePowerDown);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    bus_.writeRegister(ak8963::kRegCntl1, ak8963::kModeContinuous16bit100Hz);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

bool Ak8963::readMeasurement(MagMeasurement& out) const {
    uint8_t raw[8];
    bus_.readRegisters(ak8963::kRegSt1, raw, sizeof raw);

    if ((raw[0] & kSt1DataReady) == 0) return false;
    if ((raw[7] & kSt2Overflow) != 0) return false;

    out.x_uT = combineLittleEndian(raw[1], raw[2]) * ak8963::kMicroTeslaPerLsb;
    out.y_uT = combineLittleEndian(raw[3], raw[4]) * ak8963::kMicroTeslaPerLsb;
    out.z_uT = combineLittleEndian(raw[5], raw[6]) * ak8963::kMicroTeslaPerLsb;
    return true;
}
