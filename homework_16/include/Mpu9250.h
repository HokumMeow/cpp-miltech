#pragma once
#include <cstdint>

#include "I2cBus.h"

namespace mpu9250 {

constexpr uint8_t kDefaultAddress = 0x68;

constexpr uint8_t kRegWhoAmI = 0x75;
constexpr uint8_t kRegPwrMgmt1 = 0x6B;
//  ACCEL_XOUT_H..GYRO_ZOUT_H, 14 bytes, significant byte first.
constexpr uint8_t kRegAccelXoutH = 0x3B;
constexpr std::size_t kMeasurementBlockSize = 14;

// by default 
constexpr float kAccelSensitivityLsbPerG = 16384.0f;
constexpr float kGyroSensitivityLsbPerDps = 131.0f;

}  // namespace mpu9250

struct Mpu9250Measurement {
    float accelX_g, accelY_g, accelZ_g;
    float gyroX_dps, gyroY_dps, gyroZ_dps;
    float temperatureC;
};

// I2cBus exchange
class Mpu9250 {
public:
    explicit Mpu9250(I2cBus& bus);

    uint8_t readWhoAmI() const;
    void wake() const;  // reset sleep bit in PWR_MGMT_1 (sensor is asleep after POR)
    Mpu9250Measurement readMeasurement() const;

private:
    I2cBus& bus_;
};
