#include "source/HardwareTelemetrySource.h"

#include <cstdio>
#include <sstream>
#include <stdexcept>
#include <thread>

#include "common/Attitude.h"

namespace {

std::string hex(int v) {
    std::ostringstream oss;
    oss << "0x" << std::hex << v;
    return oss.str();
}

}  // namespace

HardwareTelemetrySource::HardwareTelemetrySource(const HardwareConfig& cfg)
    : cfg_(cfg),
      imuBus_(cfg.i2cDevice, cfg.imuAddress),
      magBus_(cfg.i2cDevice, ak8963::kAddress),
      baroBus_(cfg.i2cDevice, cfg.baroAddress),
      imu_(imuBus_),
      mag_(magBus_),
      baro_(baroBus_),
      gps_(cfg.gpsPort, cfg.gpsBaud) {
    const uint8_t imuId = imu_.readWhoAmI();
    if (!mpu9250::isKnownWhoAmI(imuId)) {
        throw std::runtime_error("MPU-9250 WHO_AM_I = " + hex(imuId) + ", expected 0x70, 0x71 or 0x73");
    }
    imu_.wake();
    imu_.enableMagBypass();

    const uint8_t magId = mag_.readWhoAmI();
    if (magId != ak8963::kWhoAmIValue) {
        throw std::runtime_error("AK8963 WHO_AM_I = " + hex(magId) + ", expected 0x48");
    }
    mag_.startContinuous();

    const uint8_t baroId = baro_.readChipId();
    if (!bmp388::isKnownChipId(baroId)) {
        throw std::runtime_error("barometer CHIP_ID = " + hex(baroId) + ", expected 0x50 (BMP388) or 0x60 (BMP390)");
    }
    baro_.configure();

    readImu();
    readBaro();

    start_ = std::chrono::steady_clock::now();
    nextTick_ = start_;
    last_.linkOk = true;
}

void HardwareTelemetrySource::readImu() {
    const Mpu9250Measurement m = imu_.readMeasurement();
    last_.imu.rollDeg = attitude::rollDeg(m.accelY_g, m.accelZ_g);
    last_.imu.pitchDeg = attitude::pitchDeg(m.accelX_g, m.accelY_g, m.accelZ_g);

    MagMeasurement mag{};
    if (mag_.readMeasurement(mag)) {  // якщо нових даних нема - лишається попередній курс
        last_.imu.headingDeg = attitude::headingDeg(mag.x_uT, mag.y_uT, cfg_.headingOffsetDeg);
    }
}

void HardwareTelemetrySource::readBaro() {
    const bmp388::Reading r = baro_.read();
    last_.baro.pressurePa = r.pressurePa;
    last_.baro.temperatureC = r.temperatureC;
    last_.baro.altitudeM = bmp388::altitudeFromPressure(r.pressurePa);
}

bool HardwareTelemetrySource::next(Telemetry& out) {
    std::this_thread::sleep_until(nextTick_);
    nextTick_ += std::chrono::milliseconds(cfg_.periodMs);

    keyboard_.poll();
    if (keyboard_.quitRequested()) return false;

    const int64_t nowMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_).count();

    try {
        readImu();
    } catch (const I2cError& e) {
        std::fprintf(stderr, "[warn] IMU: %s\n", e.what());
    }
    try {
        readBaro();
    } catch (const I2cError& e) {
        std::fprintf(stderr, "[warn] baro: %s\n", e.what());
    }

    gps_.poll(nowMs);
    last_.gps = gps_.fix(nowMs);
    last_.linkOk = keyboard_.linkOk();
    last_.timeMs = nowMs;

    out = last_;
    return true;
}
