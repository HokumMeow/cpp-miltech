#pragma once
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>

#include "sensors/Ak8963.h"
#include "sensors/Bmp388.h"
#include "sensors/GpsReceiver.h"
#include "sensors/I2cBus.h"
#include "sensors/Mpu9250.h"
#include "source/ITelemetrySource.h"
#include "source/KeyboardLink.h"

struct HardwareConfig {
    std::string i2cDevice = "/dev/i2c-1";
    std::string gpsPort = "/dev/serial0";
    int gpsBaud = 9600;
    uint8_t imuAddress = mpu9250::kDefaultAddress;
    uint8_t baroAddress = bmp388::kAddressSdoHigh;
    double headingOffsetDeg = 0.0;  // коррекція  на монтаж плати відносно носа дрона
    int periodMs = 200;             // 5 на секунду
};

class HardwareTelemetrySource : public ITelemetrySource {
public:
    // перевірка ID датчиків і запуск
    explicit HardwareTelemetrySource(const HardwareConfig& cfg);

    bool next(Telemetry& out) override;

private:
    void readImu();
    void readBaro();

    HardwareConfig cfg_;
    I2cBus imuBus_;
    I2cBus magBus_;
    I2cBus baroBus_;
    Mpu9250 imu_;
    Ak8963 mag_;
    Bmp388 baro_;
    GpsReceiver gps_;
    KeyboardLink keyboard_;

    Telemetry last_;
    std::chrono::steady_clock::time_point start_;
    std::chrono::steady_clock::time_point nextTick_;
};
