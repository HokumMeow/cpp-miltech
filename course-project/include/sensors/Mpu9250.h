#pragma once
#include <cstdint>

#include "sensors/I2cBus.h"

namespace mpu9250 {

constexpr uint8_t kDefaultAddress = 0x68;

constexpr uint8_t kRegWhoAmI = 0x75;
constexpr uint8_t kRegPwrMgmt1 = 0x6B;
constexpr uint8_t kRegIntPinCfg = 0x37;
constexpr uint8_t kBypassEnable = 0x02;
constexpr uint8_t kRegAccelXoutH = 0x3B;
constexpr std::size_t kMeasurementBlockSize = 14;

constexpr float kAccelSensitivityLsbPerG = 16384.0f;
constexpr float kGyroSensitivityLsbPerDps = 131.0f;
constexpr float kTempSensitivityLsbPerC = 333.87f;
constexpr float kTempOffsetC = 21.0f;

// 0x71 - MPU-9250, 0x73 - MPU-9255, в мене з алі віддає 0x70
inline bool isKnownWhoAmI(uint8_t id) { return id == 0x70 || id == 0x71 || id == 0x73; }

}  // namespace mpu9250

struct Mpu9250Measurement {
    float accelX_g, accelY_g, accelZ_g;
    float gyroX_dps, gyroY_dps, gyroZ_dps;
    float temperatureC;
};

// MPU-9250 драйвер з ДЗ16
class Mpu9250 {
public:
    explicit Mpu9250(I2cBus& bus);

    uint8_t readWhoAmI() const;
    void wake() const;              
    void enableMagBypass() const;   
    Mpu9250Measurement readMeasurement() const;

private:
    I2cBus& bus_;
};
