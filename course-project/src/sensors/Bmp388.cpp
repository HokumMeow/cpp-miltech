#include "sensors/Bmp388.h"

#include <chrono>
#include <cmath>
#include <thread>

namespace {

uint16_t u16(const uint8_t* b) { return static_cast<uint16_t>(b[0] | (b[1] << 8)); }
int16_t s16(const uint8_t* b) { return static_cast<int16_t>(u16(b)); }
int8_t s8(uint8_t b) { return static_cast<int8_t>(b); }

uint32_t u24(const uint8_t* b) {
    return static_cast<uint32_t>(b[0]) | (static_cast<uint32_t>(b[1]) << 8) | (static_cast<uint32_t>(b[2]) << 16);
}

constexpr uint8_t kCmdSoftReset = 0xB6;
constexpr uint8_t kPwrCtrlNormalPressTemp = 0x33;  // press_en | temp_en | mode = normal
constexpr uint8_t kOsrPress8xTemp1x = 0x03;
constexpr uint8_t kOdr25Hz = 0x03;
constexpr uint8_t kIirCoef3 = 0x02 << 1;

}  // namespace

namespace bmp388 {

Calibration parseCalibration(const uint8_t raw[kCalibSize]) {
    Calibration c{};
    // Trimming coefficients з даташита
    c.t1 = u16(raw + 0) * 256.0;
    c.t2 = u16(raw + 2) / 1073741824.0;                    // 2^30
    c.t3 = s8(raw[4]) / 281474976710656.0;                 // 2^48
    c.p1 = (s16(raw + 5) - 16384) / 1048576.0;             // 2^20
    c.p2 = (s16(raw + 7) - 16384) / 536870912.0;           // 2^29
    c.p3 = s8(raw[9]) / 4294967296.0;                      // 2^32
    c.p4 = s8(raw[10]) / 137438953472.0;                   // 2^37
    c.p5 = u16(raw + 11) * 8.0;                            // 2^-3
    c.p6 = u16(raw + 13) / 64.0;                           // 2^6
    c.p7 = s8(raw[15]) / 256.0;                            // 2^8
    c.p8 = s8(raw[16]) / 32768.0;                          // 2^15
    c.p9 = s16(raw + 17) / 281474976710656.0;              // 2^48
    c.p10 = s8(raw[19]) / 281474976710656.0;               // 2^48
    c.p11 = s8(raw[20]) / 36893488147419103232.0;          // 2^65
    return c;
}

Reading compensate(uint32_t rawTemperature, uint32_t rawPressure, const Calibration& c) {
    
    const double d1 = static_cast<double>(rawTemperature) - c.t1;
    const double d2 = d1 * c.t2;
    const double t = d2 + d1 * d1 * c.t3;

    const double up = static_cast<double>(rawPressure);
    const double out1 = c.p5 + c.p6 * t + c.p7 * t * t + c.p8 * t * t * t;
    const double out2 = up * (c.p1 + c.p2 * t + c.p3 * t * t + c.p4 * t * t * t);
    const double out3 = up * up * (c.p9 + c.p10 * t) + up * up * up * c.p11;

    return {t, out1 + out2 + out3};
}

double altitudeFromPressure(double pressurePa, double seaLevelPa) {
    return 44330.0 * (1.0 - std::pow(pressurePa / seaLevelPa, 1.0 / 5.255));
}

}  // namespace bmp388

Bmp388::Bmp388(I2cBus& bus) : bus_(bus) {}

uint8_t Bmp388::readChipId() const {
    return bus_.readRegister(bmp388::kRegChipId);
}

void Bmp388::configure() {
    bus_.writeRegister(bmp388::kRegCmd, kCmdSoftReset);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    uint8_t rawCalib[bmp388::kCalibSize];
    bus_.readRegisters(bmp388::kRegCalib, rawCalib, sizeof rawCalib);
    calib_ = bmp388::parseCalibration(rawCalib);

    bus_.writeRegister(bmp388::kRegOsr, kOsrPress8xTemp1x);
    bus_.writeRegister(bmp388::kRegOdr, kOdr25Hz);
    bus_.writeRegister(bmp388::kRegIirConfig, kIirCoef3);
    bus_.writeRegister(bmp388::kRegPwrCtrl, kPwrCtrlNormalPressTemp);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

bmp388::Reading Bmp388::read() const {
    uint8_t raw[6];
    bus_.readRegisters(bmp388::kRegData, raw, sizeof raw);
    return bmp388::compensate(u24(raw + 3), u24(raw), calib_);
}
