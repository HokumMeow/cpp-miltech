#pragma once
#include <cstdint>

#include "sensors/I2cBus.h"

namespace ak8963 {

constexpr uint8_t kAddress = 0x0C;

constexpr uint8_t kRegWhoAmI = 0x00;   // повинен повернути 0x48
constexpr uint8_t kWhoAmIValue = 0x48;
constexpr uint8_t kRegSt1 = 0x02;      // біт 0 = DRDY
constexpr uint8_t kRegCntl1 = 0x0A;
constexpr uint8_t kModePowerDown = 0x00;
constexpr uint8_t kModeContinuous16bit100Hz = 0x16;  // біт 4 = 16 біт, 0110 = 100 Гц

constexpr float kMicroTeslaPerLsb = 0.15f;  // 16 біт 4912 мкТл на 32760

}  // namespace ak8963

struct MagMeasurement {
    float x_uT, y_uT, z_uT;
};

// AK8963 всередині GY-9250
class Ak8963 {
public:
    explicit Ak8963(I2cBus& bus);

    uint8_t readWhoAmI() const;
    void startContinuous() const;

    bool readMeasurement(MagMeasurement& out) const;

private:
    I2cBus& bus_;
};
