// ============================================================================
// ТЕСТИ: читання сценаріїв і запис логу
// ============================================================================
// Перевіряємо, що криві дані не проходять мовчки: мало колонок, нечисло,
// час, що не зростає, порожній файл, відсутній файл - усе це виняток.
//
// головний тут - WrittenFileCanBeReplayed: записуємо телеметрію логером і
// одразу читаємо назад джерелом відтворення. Це перевірка того, що формат
// логу і формат сценарію - справді один формат, а не два схожих. Саме на цьому
// тримається можливість відтворити реальний політ удома.
// ============================================================================

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "logging/RouteLogger.h"
#include "source/ReplayTelemetrySource.h"

TEST(Replay, ReadsRowsAndSkipsHeaderCommentsAndBlankLines) {
    std::istringstream in(
        "t_ms,lat,lon,alt_m,heading_deg,link,sats,state\n"
        "# коментар\n"
        "\n"
        "0,50.4501000,30.5234000,150.00,45.0,1,8,NORMAL\n"
        "500,50.4502000,30.5235000,151.50,46.0,0,7\n");

    ReplayTelemetrySource src(in, "test");
    ASSERT_EQ(src.size(), 2u);

    Telemetry t;
    ASSERT_TRUE(src.next(t));
    EXPECT_EQ(t.timeMs, 0);
    EXPECT_NEAR(t.gps.latDeg, 50.4501, 1e-9);
    EXPECT_NEAR(t.gps.lonDeg, 30.5234, 1e-9);
    EXPECT_NEAR(t.baro.altitudeM, 150.0, 1e-9);
    EXPECT_NEAR(t.imu.headingDeg, 45.0, 1e-9);
    EXPECT_TRUE(t.linkOk);
    EXPECT_EQ(t.gps.satellites, 8);
    EXPECT_TRUE(t.gps.valid);

    ASSERT_TRUE(src.next(t));
    EXPECT_EQ(t.timeMs, 500);
    EXPECT_FALSE(t.linkOk);

    EXPECT_FALSE(src.next(t));  // дані закінчились
}

TEST(Replay, SensorColumnsAreReadWhenPresent) {
    std::istringstream in(
        "t_ms,lat,lon,alt_m,heading_deg,link,sats,state,roll_deg,pitch_deg,temp_c\n"
        "0,50.0,30.0,150.00,45.0,1,8,NORMAL,-12.5,7.5,24.3\n");

    ReplayTelemetrySource src(in, "test");
    Telemetry t;
    ASSERT_TRUE(src.next(t));
    EXPECT_NEAR(t.imu.rollDeg, -12.5, 1e-9);
    EXPECT_NEAR(t.imu.pitchDeg, 7.5, 1e-9);
    EXPECT_NEAR(t.baro.temperatureC, 24.3, 1e-9);
}

// Старі сценарії з 7 колонок читаються як і раніше, кути й температура лишаються нулями.
TEST(Replay, SensorColumnsAreOptional) {
    std::istringstream in("0,50.0,30.0,150.00,45.0,1,8\n");
    ReplayTelemetrySource src(in, "test");
    Telemetry t;
    ASSERT_TRUE(src.next(t));
    EXPECT_DOUBLE_EQ(t.imu.rollDeg, 0.0);
    EXPECT_DOUBLE_EQ(t.baro.temperatureC, 0.0);
}

TEST(Replay, ZeroSatellitesMeansNoFix) {
    std::istringstream in("0,0,0,0,0,1,0\n");
    ReplayTelemetrySource src(in, "test");
    Telemetry t;
    ASSERT_TRUE(src.next(t));
    EXPECT_FALSE(t.gps.valid);
}

TEST(Replay, RejectsTooFewColumns) {
    std::istringstream in("0,50.0,30.0\n");
    EXPECT_THROW(ReplayTelemetrySource(in, "test"), std::runtime_error);
}

TEST(Replay, RejectsInvalidNumber) {
    std::istringstream in("0,50.0,30.0,100.0,abc,1,8\n");
    EXPECT_THROW(ReplayTelemetrySource(in, "test"), std::runtime_error);
}

TEST(Replay, RejectsNonIncreasingTime) {
    std::istringstream in(
        "1000,50.0,30.0,100.0,0.0,1,8\n"
        "1000,50.0,30.0,100.0,0.0,1,8\n");
    EXPECT_THROW(ReplayTelemetrySource(in, "test"), std::runtime_error);
}

TEST(Replay, RejectsEmptyFile) {
    std::istringstream in("t_ms,lat,lon,alt_m,heading_deg,link,sats,state\n");
    EXPECT_THROW(ReplayTelemetrySource(in, "test"), std::runtime_error);
}

TEST(Replay, MissingFileThrows) {
    EXPECT_THROW(ReplayTelemetrySource("/nonexistent/scenario.csv"), std::runtime_error);
}

TEST(RouteLogger, TracksPathLengthAndMaxDistanceFromHome) {
    RouteLogger logger;  // без файлу
    logger.setHome({50.0, 30.0});

    Telemetry t;
    t.gps.valid = true;
    t.gps.satellites = 8;
    t.gps.lonDeg = 30.0;

    // 0 м -> 111.32 м на північ -> назад у 55.66 м від дому
    t.gps.latDeg = 50.0;
    logger.log(t, FlightState::Normal);
    t.gps.latDeg = 50.001;
    logger.log(t, FlightState::Normal);
    t.gps.latDeg = 50.0005;
    logger.log(t, FlightState::Return);

    EXPECT_EQ(logger.stats().points, 3);
    EXPECT_NEAR(logger.stats().pathLengthM, 111.32 + 55.66, 0.05);
    EXPECT_NEAR(logger.stats().maxDistanceFromHomeM, 111.32, 0.01);
}

TEST(RouteLogger, FixlessSamplesAreNotCountedInPath) {
    RouteLogger logger;
    Telemetry t;
    t.gps.valid = false;
    logger.log(t, FlightState::WaitingForFix);
    EXPECT_EQ(logger.stats().points, 1);
    EXPECT_DOUBLE_EQ(logger.stats().pathLengthM, 0.0);
}

TEST(RouteLogger, WrittenFileCanBeReplayed) {
    const std::string path = (std::filesystem::temp_directory_path() / "rth_logger_roundtrip.csv").string();

    Telemetry a;
    a.timeMs = 200;
    a.gps.valid = true;
    a.gps.satellites = 9;
    a.gps.latDeg = 50.4501234;
    a.gps.lonDeg = 30.5234567;
    a.baro.altitudeM = 155.25;
    a.imu.headingDeg = 123.4;
    a.imu.rollDeg = -12.5;
    a.imu.pitchDeg = 7.5;
    a.baro.temperatureC = 24.3;
    a.linkOk = false;

    {
        RouteLogger logger(path);
        logger.log(a, FlightState::LinkLost);
    }

    ReplayTelemetrySource src(path);
    Telemetry b;
    ASSERT_TRUE(src.next(b));
    EXPECT_EQ(b.timeMs, 200);
    EXPECT_NEAR(b.gps.latDeg, 50.4501234, 1e-7);
    EXPECT_NEAR(b.gps.lonDeg, 30.5234567, 1e-7);
    EXPECT_NEAR(b.baro.altitudeM, 155.25, 1e-2);
    EXPECT_NEAR(b.imu.headingDeg, 123.4, 1e-1);
    EXPECT_FALSE(b.linkOk);
    EXPECT_EQ(b.gps.satellites, 9);
    // дані датчиків теж переживають запис і читання
    EXPECT_NEAR(b.imu.rollDeg, -12.5, 1e-1);
    EXPECT_NEAR(b.imu.pitchDeg, 7.5, 1e-1);
    EXPECT_NEAR(b.baro.temperatureC, 24.3, 1e-1);

    std::remove(path.c_str());
}

TEST(RouteLogger, UnwritablePathThrows) {
    EXPECT_THROW(RouteLogger("/nonexistent_dir/route.csv"), std::runtime_error);
}
