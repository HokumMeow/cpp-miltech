#include "Mpu9250.h"

namespace {

int16_t combine(uint8_t hi, uint8_t lo) {
    return static_cast<int16_t>((static_cast<uint16_t>(hi) << 8) | lo);
}

}  // namespace

Mpu9250::Mpu9250(I2cBus& bus) : bus_(bus) {}

uint8_t Mpu9250::readWhoAmI() const {
    return bus_.readRegister(mpu9250::kRegWhoAmI);
}

void Mpu9250::wake() const {
    bus_.writeRegister(mpu9250::kRegPwrMgmt1, 0x00);
}

Mpu9250Measurement Mpu9250::readMeasurement() const {
    uint8_t raw[mpu9250::kMeasurementBlockSize];
    bus_.readRegisters(mpu9250::kRegAccelXoutH, raw, sizeof raw);

    const int16_t accelX = combine(raw[0], raw[1]);
    const int16_t accelY = combine(raw[2], raw[3]);
    const int16_t accelZ = combine(raw[4], raw[5]);
    const int16_t temp   = combine(raw[6], raw[7]);
    const int16_t gyroX  = combine(raw[8], raw[9]);
    const int16_t gyroY  = combine(raw[10], raw[11]);
    const int16_t gyroZ  = combine(raw[12], raw[13]);

    Mpu9250Measurement m{};
    m.accelX_g = accelX / mpu9250::kAccelSensitivityLsbPerG;
    m.accelY_g = accelY / mpu9250::kAccelSensitivityLsbPerG;
    m.accelZ_g = accelZ / mpu9250::kAccelSensitivityLsbPerG;
    m.gyroX_dps = gyroX / mpu9250::kGyroSensitivityLsbPerDps;
    m.gyroY_dps = gyroY / mpu9250::kGyroSensitivityLsbPerDps;
    m.gyroZ_dps = gyroZ / mpu9250::kGyroSensitivityLsbPerDps;
    m.temperatureC = temp / 340.0f + 36.53f;
    return m;
}
