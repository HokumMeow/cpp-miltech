#pragma once
#include <cstdint>

#include "sensors/I2cBus.h"

namespace bmp388 {

constexpr uint8_t kAddressSdoLow = 0x76;
constexpr uint8_t kAddressSdoHigh = 0x77;

constexpr uint8_t kRegChipId = 0x00;
constexpr uint8_t kChipIdBmp388 = 0x50;
constexpr uint8_t kChipIdBmp390 = 0x60;  // Bmp390 ті ж регістри але інший ід

inline bool isKnownChipId(uint8_t id) { return id == kChipIdBmp388 || id == kChipIdBmp390; }
constexpr uint8_t kRegData = 0x04;       // 3 байти тиск + 3 байти темпер. молодший байт першим
constexpr uint8_t kRegPwrCtrl = 0x1B;
constexpr uint8_t kRegOsr = 0x1C;
constexpr uint8_t kRegOdr = 0x1D;
constexpr uint8_t kRegIirConfig = 0x1F;
constexpr uint8_t kRegCalib = 0x31;      // 21 байт заводські коефіцієнти
constexpr uint8_t kRegCmd = 0x7E;
constexpr std::size_t kCalibSize = 21;

// Коефіцієнти з NVM перераховані у double за формулами з даташита
struct Calibration {
    double t1, t2, t3;
    double p1, p2, p3, p4, p5, p6, p7, p8, p9, p10, p11;
};

struct Reading {
    double temperatureC;
    double pressurePa;
};

Calibration parseCalibration(const uint8_t raw[kCalibSize]);

// Компенсація сирих відліків
// формули з даташита варіант з плаваючою комою
Reading compensate(uint32_t rawTemperature, uint32_t rawPressure, const Calibration& c);

double altitudeFromPressure(double pressurePa, double seaLevelPa = 101325.0);

}  // namespace bmp388

class Bmp388 {
public:
    explicit Bmp388(I2cBus& bus);

    uint8_t readChipId() const;
    
    void configure();
    bmp388::Reading read() const;

private:
    I2cBus& bus_;
    bmp388::Calibration calib_{};
};
