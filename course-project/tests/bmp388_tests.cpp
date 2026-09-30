// ============================================================================
// ТЕСТИ: барометр
// ============================================================================
// Перевіряємо окремо три речі:
//   * розпізнавання чипа (BMP388 / BMP390 приймаємо, BMP280 - ні);
//   * барометричну формулу по контрольних точках стандартної атмосфери
//     (на рівні моря - 0 м; 89874.6 Па - приблизно 1000 м; 120 Па нижче
//     нормального тиску - приблизно 10 м);
//   * розбір калібрувальних коефіцієнтів і самі поліноми компенсації.
//
// Останнє зроблено так: підставляємо коефіцієнти, при яких громіздкий поліном
// вироджується в просту формулу, і звіряємо з порахованим на папері. Це ловить
// помилку в масштабному дільнику або зсув на байт у розборі калібрування.
// ============================================================================

#include <gtest/gtest.h>

#include <cstring>

#include "sensors/Bmp388.h"

TEST(Bmp388, AcceptsBmp388AndBmp390ChipIds) {
    EXPECT_TRUE(bmp388::isKnownChipId(0x50));
    EXPECT_TRUE(bmp388::isKnownChipId(0x60));
    EXPECT_FALSE(bmp388::isKnownChipId(0x58));  // BMP280
    EXPECT_FALSE(bmp388::isKnownChipId(0x00));
}

TEST(Bmp388, AltitudeAtSeaLevelPressureIsZero) {
    EXPECT_NEAR(bmp388::altitudeFromPressure(101325.0), 0.0, 1e-9);
}

TEST(Bmp388, AltitudeMatchesStandardAtmosphere) {
    // за стандартною атмосферою 1000 м відповідає ~89875 Па
    EXPECT_NEAR(bmp388::altitudeFromPressure(89874.6), 1000.0, 1.0);
}

TEST(Bmp388, AltitudeGrowsWhenPressureFalls) {
    EXPECT_GT(bmp388::altitudeFromPressure(100000.0), bmp388::altitudeFromPressure(101000.0));
}

TEST(Bmp388, TenMetersNearSeaLevelIsAbout120Pascal) {
    const double p0 = 101325.0;
    const double h = bmp388::altitudeFromPressure(p0 - 120.0);
    EXPECT_NEAR(h, 10.0, 0.5);
}

TEST(Bmp388, ParseCalibrationAppliesScaleFactors) {
    uint8_t raw[bmp388::kCalibSize];
    std::memset(raw, 0, sizeof raw);
    raw[0] = 0x10;  // T1 = 16 (молодший байт першим)
    raw[2] = 0x00;  // T2 = 0x4000 = 16384
    raw[3] = 0x40;
    raw[4] = 0xFF;  // T3 = -1
    raw[5] = 0x00;  // P1 = 0x4000 -> (16384 - 16384) = 0
    raw[6] = 0x40;
    raw[11] = 0x08;  // P5 = 8

    const bmp388::Calibration c = bmp388::parseCalibration(raw);
    EXPECT_DOUBLE_EQ(c.t1, 16.0 * 256.0);
    EXPECT_DOUBLE_EQ(c.t2, 16384.0 / 1073741824.0);
    EXPECT_DOUBLE_EQ(c.t3, -1.0 / 281474976710656.0);
    EXPECT_DOUBLE_EQ(c.p1, 0.0);
    EXPECT_DOUBLE_EQ(c.p5, 64.0);
}

TEST(Bmp388, TemperatureCompensationGivesPlausibleValue) {
    // Умовні, але правдоподібні коефіцієнти: T1=27000, T2=19000, T3=0.
    bmp388::Calibration c{};
    c.t1 = 27000.0 * 256.0;
    c.t2 = 19000.0 / 1073741824.0;
    c.t3 = 0.0;

    // (8'500'000 - 6'912'000) * 19000 / 2^30 = 28.1 градуса
    const bmp388::Reading r = bmp388::compensate(8'500'000, 0, c);
    EXPECT_NEAR(r.temperatureC, 28.1, 0.1);
}

TEST(Bmp388, PressureCompensationUsesLinearTermsWhenOthersAreZero) {
    // Якщо всі коефіцієнти, крім P1 і P5, нульові, тиск = P5 + raw * P1.
    bmp388::Calibration c{};
    c.p1 = 0.5;
    c.p5 = 100.0;

    const bmp388::Reading r = bmp388::compensate(0, 200000, c);
    EXPECT_NEAR(r.pressurePa, 100.0 + 200000 * 0.5, 1e-6);
}
