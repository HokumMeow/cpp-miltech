// ============================================================================
// ТЕСТИ: розбір NMEA
// ============================================================================
// Речення взяті справжні, з правильними контрольними сумами. Перевіряємо:
//   * формат ddmm.mmmm розкладається правильно (5027.0060 -> 50.4501, а не 50.27);
//   * південь і захід дають мінус;
//   * статус 'V' знімає ознаку фіксації;
//   * биту контрольну суму відкидаємо;
//   * невідомі речення (GSV) ігноруємо мовчки - їх у потоці більшість;
//   * сміття, порожній рядок і обрізане речення не валять програму.
// ============================================================================

#include <gtest/gtest.h>

#include "sensors/NmeaParser.h"

namespace {
const char* kRmcFix = "$GNRMC,123519.00,A,5027.0060,N,03031.4040,E,1.94,84.4,230394,,,A*41";
const char* kRmcNoFix = "$GNRMC,123519.00,V,,,,,,,230394,,,N*61";
const char* kGga = "$GNGGA,123519.00,5027.0060,N,03031.4040,E,1,08,0.9,150.5,M,0.0,M,,*48";
const char* kRmcSouthWest = "$GPRMC,123519,A,3352.0000,S,15112.0000,W,0.0,0.0,230394,,*11";
const char* kGsv = "$GPGSV,3,1,11,03,03,111,00,04,15,270,00,06,01,010,00,13,06,292,00*74";
}  // namespace

TEST(NmeaParser, ParsesRmcPositionAndSpeed) {
    NmeaParser p;
    EXPECT_TRUE(p.parseLine(kRmcFix));
    EXPECT_TRUE(p.fix().valid);
    // 50 градусів 27.0060 хвилин = 50.4501
    EXPECT_NEAR(p.fix().latDeg, 50.4501, 1e-6);
    EXPECT_NEAR(p.fix().lonDeg, 30.5234, 1e-6);
    EXPECT_NEAR(p.fix().speedMps, 1.94 * 0.514444, 1e-3);
}

TEST(NmeaParser, RmcWithoutFixIsNotValid) {
    NmeaParser p;
    p.parseLine(kRmcFix);
    EXPECT_TRUE(p.parseLine(kRmcNoFix));
    EXPECT_FALSE(p.fix().valid);
}

TEST(NmeaParser, GgaGivesSatelliteCount) {
    NmeaParser p;
    EXPECT_TRUE(p.parseLine(kGga));
    EXPECT_EQ(p.fix().satellites, 8);
}

TEST(NmeaParser, SouthAndWestAreNegative) {
    NmeaParser p;
    EXPECT_TRUE(p.parseLine(kRmcSouthWest));
    EXPECT_NEAR(p.fix().latDeg, -(33.0 + 52.0 / 60.0), 1e-6);
    EXPECT_NEAR(p.fix().lonDeg, -(151.0 + 12.0 / 60.0), 1e-6);
}

TEST(NmeaParser, RejectsWrongChecksum) {
    NmeaParser p;
    EXPECT_FALSE(p.parseLine("$GNRMC,123519.00,A,5027.0060,N,03031.4040,E,1.94,84.4,230394,,,A*00"));
    EXPECT_FALSE(p.fix().valid);
}

TEST(NmeaParser, IgnoresUnknownSentences) {
    NmeaParser p;
    EXPECT_FALSE(p.parseLine(kGsv));
}

TEST(NmeaParser, RejectsGarbage) {
    NmeaParser p;
    EXPECT_FALSE(p.parseLine(""));
    EXPECT_FALSE(p.parseLine("hello"));
    EXPECT_FALSE(p.parseLine("$GNRMC,no,checksum"));
    EXPECT_FALSE(p.parseLine("$GNRMC*4"));
}
