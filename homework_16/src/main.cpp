#include <chrono>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "I2cBus.h"
#include "Mpu9250.h"

namespace {

constexpr auto kPollInterval = std::chrono::milliseconds(200);  // 5 разів на секунду

// id check
bool isKnownMpu9250Id(uint8_t id) {
    return id == 0x71 || id == 0x73;
}

uint8_t parseAddress(const std::string& s) {
    return static_cast<uint8_t>(std::stoul(s, nullptr, 0));
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string devicePath = argc > 1 ? argv[1] : "/dev/i2c-1";
    const uint8_t address = argc > 2 ? parseAddress(argv[2]) : mpu9250::kDefaultAddress;

    std::unique_ptr<I2cBus> bus;
    try {
        bus = std::make_unique<I2cBus>(devicePath, address);
    } catch (const I2cError& e) {
        std::cerr << "[fatal] " << e.what() << std::endl;
        return 1;
    }

    Mpu9250 sensor(*bus);

    try {
        const uint8_t whoAmI = sensor.readWhoAmI();
        if (isKnownMpu9250Id(whoAmI)) {
            std::printf("WHO_AM_I = 0x%02X (MPU-9250 confirmed)\n", whoAmI);
        } else {
            std::printf("[warn] WHO_AM_I = 0x%02X, expected 0x71/0x73 — probably not the right device\n", whoAmI);
        }
        sensor.wake();
    } catch (const I2cError& e) {
        std::cerr << "[error] " << e.what() << std::endl;
    }

    while (true) {
        try {
            const Mpu9250Measurement m = sensor.readMeasurement();
            std::printf("\raccel[g] x=%+.3f y=%+.3f z=%+.3f  gyro[°/s] x=%+7.2f y=%+7.2f z=%+7.2f  temp=%.1f°C   ",
                        m.accelX_g, m.accelY_g, m.accelZ_g,
                        m.gyroX_dps, m.gyroY_dps, m.gyroZ_dps,
                        m.temperatureC);
            std::fflush(stdout);
        } catch (const I2cError& e) {
            std::printf("\n[error] %s\n", e.what());
        }
        std::this_thread::sleep_for(kPollInterval);
    }
}
