// ============================================================================
// ТЕСТИ: навігація
// ============================================================================
// Дім у (50.0, 30.0), дрон на 0.001 градуса північніше - тобто рівно 111 метрів
// на північ. Звідси всі очікувані числа рахуються в голові: дім на півдні,
// азимут 180.
// ============================================================================

#include <gtest/gtest.h>

#include <cmath>

#include "nav/Navigator.h"

namespace {

// Дім у (50.0, 30.0), висота дому 100 м. Дрон стоїть на північ від дому на 0.001 градуса (~111 м).
Telemetry sample(int64_t timeMs, double relAltM, bool linkOk, double headingDeg,
                 double lat = 50.001, int sats = 8) {
    Telemetry t;
    t.timeMs = timeMs;
    t.gps.valid = sats > 0;
    t.gps.satellites = sats;
    t.gps.latDeg = lat;
    t.gps.lonDeg = 30.0;
    t.baro.altitudeM = 100.0 + relAltM;
    t.imu.headingDeg = headingDeg;
    t.linkOk = linkOk;
    return t;
}

FailsafeFsm fsmInState(FlightState target) {
    FailsafeFsm fsm;
    fsm.update(sample(0, 0.0, true, 0.0, 50.0));  // дім
    if (target == FlightState::Normal) return fsm;

    fsm.update(sample(1000, 10.0, false, 0.0));
    fsm.update(sample(4000, 10.0, false, 0.0));  // Climb
    if (target == FlightState::Climb) return fsm;

    fsm.update(sample(4200, 30.0, false, 0.0));  // Return
    return fsm;
}

}  // namespace

TEST(Nav, ManualWhileNormal) {
    FailsafeFsm fsm = fsmInState(FlightState::Normal);
    const NavCommand cmd = computeNav(fsm, sample(500, 10.0, true, 0.0));
    EXPECT_EQ(cmd.action, NavAction::Manual);
    EXPECT_NEAR(cmd.distanceToHomeM, 111.32, 0.01);  // відстань показуємо і в ручному режимі
}

TEST(Nav, NoHomeMeansManualAndZeroes) {
    FailsafeFsm fsm;
    const NavCommand cmd = computeNav(fsm, sample(0, 0.0, true, 0.0));
    EXPECT_EQ(cmd.action, NavAction::Manual);
    EXPECT_DOUBLE_EQ(cmd.distanceToHomeM, 0.0);
}

TEST(Nav, ClimbCommandReportsMissingHeight) {
    FailsafeFsm fsm = fsmInState(FlightState::Climb);
    const NavCommand cmd = computeNav(fsm, sample(4100, 12.0, false, 0.0));
    EXPECT_EQ(cmd.action, NavAction::Climb);
    EXPECT_NEAR(cmd.climbM, 18.0, 1e-9);  // 30 - 12
}

TEST(Nav, FlyHomeHeadingSouthWhenHomeIsSouth) {
    FailsafeFsm fsm = fsmInState(FlightState::Return);
    ASSERT_EQ(fsm.state(), FlightState::Return);

    // дім на півдні (азимут 180), ніс дивиться на північ: розвернутись на 180
    NavCommand cmd = computeNav(fsm, sample(4400, 30.0, false, 0.0));
    EXPECT_EQ(cmd.action, NavAction::FlyHome);
    EXPECT_NEAR(cmd.bearingToHomeDeg, 180.0, 0.01);
    EXPECT_NEAR(std::abs(cmd.headingErrorDeg), 180.0, 0.01);

    // ніс уже на південь: поправки нема
    cmd = computeNav(fsm, sample(4400, 30.0, false, 180.0));
    EXPECT_NEAR(cmd.headingErrorDeg, 0.0, 0.01);
}

TEST(Nav, TurnDirectionSign) {
    FailsafeFsm fsm = fsmInState(FlightState::Return);

    // ніс на схід (90), дім на півдні (180): повернути праворуч на 90
    EXPECT_NEAR(computeNav(fsm, sample(4400, 30.0, false, 90.0)).headingErrorDeg, 90.0, 0.01);
    // ніс на захід (270), дім на півдні (180): повернути ліворуч на 90
    EXPECT_NEAR(computeNav(fsm, sample(4400, 30.0, false, 270.0)).headingErrorDeg, -90.0, 0.01);
}

TEST(Nav, HoldWhenReturningWithoutGps) {
    FailsafeFsm fsm = fsmInState(FlightState::Return);
    const NavCommand cmd = computeNav(fsm, sample(4400, 30.0, false, 0.0, 50.001, 0));
    EXPECT_EQ(cmd.action, NavAction::Hold);
}

// Без координат дому навігація не має що рахувати: у Return це HOLD, а не FLY_HOME.
TEST(Nav, HoldWhenHomeHasNoPosition) {
    FailsafeConfig cfg;
    cfg.requireGpsForHome = false;
    cfg.rthAltitudeM = 1.0;
    cfg.altToleranceM = 0.3;
    FailsafeFsm fsm(cfg);

    fsm.update(sample(0, 0.0, true, 0.0, 50.0, 0));
    fsm.update(sample(1000, 0.0, false, 0.0, 50.0, 0));
    fsm.update(sample(4000, 0.0, false, 0.0, 50.0, 0));
    fsm.update(sample(4200, 0.8, false, 0.0, 50.0, 0));
    ASSERT_EQ(fsm.state(), FlightState::Return);

    // GPS навіть є, але дім запам'ятали без координат - летіти нікуди
    const NavCommand cmd = computeNav(fsm, sample(4400, 0.8, false, 0.0, 50.001, 8));
    EXPECT_EQ(cmd.action, NavAction::Hold);
    EXPECT_DOUBLE_EQ(cmd.distanceToHomeM, 0.0);
    EXPECT_NEAR(cmd.climbM, 0.2, 1e-9);  // висота рахується, бо вона в дома є
}

TEST(Nav, LandWhenHome) {
    FailsafeFsm fsm = fsmInState(FlightState::Return);
    fsm.update(sample(4600, 30.0, false, 180.0, 50.00001));
    ASSERT_EQ(fsm.state(), FlightState::Home);
    EXPECT_EQ(computeNav(fsm, sample(4800, 30.0, false, 180.0, 50.00001)).action, NavAction::Land);
}
