// ============================================================================
// ТЕСТИ: автомат failsafe
// ============================================================================
// Тести проходять усі переходи і, крайні випадки:
//   * без фіксації і з трьома супутниками дім не запам'ятовується;
//   * зв'язок, що повернувся ДО таймауту, скасовує тривогу;
//   * межа таймауту перевірена з обох боків: 2999 мс - ще ні, 3000 - уже так;
//   * після старту RTH повернення зв'язку вже нічого не скасовує;
//   * втрата GPS у RETURN не викидає з режиму повернення;
//   * HOME - кінцевий стан, вийти з нього не можна;
//   * режим --no-gps: дім тільки за висотою, і RTH чесно зупиняється на RETURN.
//
// ============================================================================

#include <gtest/gtest.h>

#include "failsafe/FailsafeFsm.h"

namespace {

// Дім у точці (50.0, 30.0). 0.001 градуса широти = 111.32 м на північ.
constexpr double kHomeLat = 50.0;
constexpr double kHomeLon = 30.0;

Telemetry sample(int64_t timeMs, double relAltM, bool linkOk, double lat = kHomeLat, int sats = 8) {
    Telemetry t;
    t.timeMs = timeMs;
    t.gps.valid = sats > 0;
    t.gps.satellites = sats;
    t.gps.latDeg = lat;
    t.gps.lonDeg = kHomeLon;
    t.baro.altitudeM = 100.0 + relAltM;  // висота дому = 100 м
    t.linkOk = linkOk;
    return t;
}

// Автомат, у якому вже запам'ятано дім і стан Normal.
FailsafeFsm armedFsm() {
    FailsafeFsm fsm;
    fsm.update(sample(0, 0.0, true));
    return fsm;
}

// Автомат у стані Climb: зв'язок втрачено на 1 с і минув таймаут.
FailsafeFsm climbingFsm() {
    FailsafeFsm fsm = armedFsm();
    fsm.update(sample(1000, 10.0, false));
    fsm.update(sample(4000, 10.0, false));
    return fsm;
}

}  // namespace

TEST(Failsafe, StartsWaitingForFix) {
    FailsafeFsm fsm;
    EXPECT_EQ(fsm.state(), FlightState::WaitingForFix);
    EXPECT_FALSE(fsm.hasHome());
}

TEST(Failsafe, StaysWaitingWithoutEnoughSatellites) {
    FailsafeFsm fsm;
    fsm.update(sample(0, 0.0, true, kHomeLat, 0));
    fsm.update(sample(200, 0.0, true, kHomeLat, 3));
    EXPECT_EQ(fsm.state(), FlightState::WaitingForFix);
    EXPECT_FALSE(fsm.hasHome());
}

TEST(Failsafe, FixSetsHomeAndStartsNormalFlight) {
    FailsafeFsm fsm;
    fsm.update(sample(0, 0.0, true, kHomeLat, 6));
    EXPECT_EQ(fsm.state(), FlightState::Normal);
    ASSERT_TRUE(fsm.hasHome());
    EXPECT_DOUBLE_EQ(fsm.home().latDeg, kHomeLat);
    EXPECT_DOUBLE_EQ(fsm.homeAltitudeM(), 100.0);
}

TEST(Failsafe, LinkLossStartsCountdown) {
    FailsafeFsm fsm = armedFsm();
    fsm.update(sample(1000, 10.0, false));
    EXPECT_EQ(fsm.state(), FlightState::LinkLost);
}

TEST(Failsafe, LinkReturningBeforeTimeoutCancelsFailsafe) {
    FailsafeFsm fsm = armedFsm();
    fsm.update(sample(1000, 10.0, false));
    fsm.update(sample(2000, 10.0, false));
    fsm.update(sample(3000, 10.0, true));
    EXPECT_EQ(fsm.state(), FlightState::Normal);
}

TEST(Failsafe, TimeoutStartsClimb) {
    FailsafeFsm fsm = armedFsm();
    fsm.update(sample(1000, 10.0, false));
    fsm.update(sample(3999, 10.0, false));
    EXPECT_EQ(fsm.state(), FlightState::LinkLost);  // 2999 мс < 3000 мс
    fsm.update(sample(4000, 10.0, false));
    EXPECT_EQ(fsm.state(), FlightState::Climb);
}

TEST(Failsafe, ClimbWaitsForReturnAltitude) {
    FailsafeFsm fsm = climbingFsm();
    fsm.update(sample(4200, 20.0, false));
    EXPECT_EQ(fsm.state(), FlightState::Climb);
    fsm.update(sample(4400, 28.0, false));  // 30 - допуск 2 = 28
    EXPECT_EQ(fsm.state(), FlightState::Return);
}

TEST(Failsafe, ReturnFinishesWithinHomeRadius) {
    FailsafeFsm fsm = climbingFsm();
    fsm.update(sample(4400, 30.0, false, kHomeLat + 0.001));  // 111 м від дому
    EXPECT_EQ(fsm.state(), FlightState::Return);
    fsm.update(sample(5000, 30.0, false, kHomeLat + 0.0001)); // 11 м
    EXPECT_EQ(fsm.state(), FlightState::Return);
    fsm.update(sample(6000, 30.0, false, kHomeLat + 0.00002)); // 2.2 м
    EXPECT_EQ(fsm.state(), FlightState::Home);
}

TEST(Failsafe, FailsafeIsLatchedWhenLinkComesBack) {
    FailsafeFsm fsm = climbingFsm();
    fsm.update(sample(4400, 30.0, true, kHomeLat + 0.001));
    EXPECT_EQ(fsm.state(), FlightState::Return);
    fsm.update(sample(4600, 30.0, true, kHomeLat + 0.001));
    EXPECT_EQ(fsm.state(), FlightState::Return);
}

TEST(Failsafe, ReturnWaitsIfGpsIsLost) {
    FailsafeFsm fsm = climbingFsm();
    fsm.update(sample(4400, 30.0, false, kHomeLat + 0.001));
    ASSERT_EQ(fsm.state(), FlightState::Return);
    fsm.update(sample(4600, 30.0, false, kHomeLat, 0));  // GPS зник, координати ігноруємо
    EXPECT_EQ(fsm.state(), FlightState::Return);
}

// --no-gps: перевірка на столі. Дім запам'ятовується за висотою, координат нема.
TEST(Failsafe, WithoutGpsRequirementHomeIsSetFromBarometerAlone) {
    FailsafeConfig cfg;
    cfg.requireGpsForHome = false;
    FailsafeFsm fsm(cfg);

    fsm.update(sample(0, 0.0, true, kHomeLat, 0));  // супутників нема
    EXPECT_EQ(fsm.state(), FlightState::Normal);
    EXPECT_TRUE(fsm.hasHome());
    EXPECT_FALSE(fsm.homeHasPosition());
    EXPECT_DOUBLE_EQ(fsm.homeAltitudeM(), 100.0);
}

// Без координат дому RTH доходить до Return і там лишається: прибуття перевірити нічим.
TEST(Failsafe, WithoutGpsRthStopsAtReturn) {
    FailsafeConfig cfg;
    cfg.requireGpsForHome = false;
    cfg.rthAltitudeM = 1.0;
    cfg.altToleranceM = 0.3;
    FailsafeFsm fsm(cfg);

    fsm.update(sample(0, 0.0, true, kHomeLat, 0));
    fsm.update(sample(1000, 0.0, false, kHomeLat, 0));
    EXPECT_EQ(fsm.state(), FlightState::LinkLost);

    fsm.update(sample(4000, 0.0, false, kHomeLat, 0));
    EXPECT_EQ(fsm.state(), FlightState::Climb);

    fsm.update(sample(4200, 0.5, false, kHomeLat, 0));  // піднято на 0.5 м - ще мало
    EXPECT_EQ(fsm.state(), FlightState::Climb);

    fsm.update(sample(4400, 0.8, false, kHomeLat, 0));  // 1.0 - допуск 0.3 = 0.7
    EXPECT_EQ(fsm.state(), FlightState::Return);

    // навіть якщо координати "з'являться", дім без позиції - Home не настає
    fsm.update(sample(5000, 0.8, false, kHomeLat, 8));
    EXPECT_EQ(fsm.state(), FlightState::Return);
}

TEST(Failsafe, HomeIsFinal) {
    FailsafeFsm fsm = climbingFsm();
    fsm.update(sample(4400, 30.0, false));
    fsm.update(sample(4600, 30.0, false));
    ASSERT_EQ(fsm.state(), FlightState::Home);
    fsm.update(sample(4800, 30.0, false, kHomeLat + 0.001));
    EXPECT_EQ(fsm.state(), FlightState::Home);
}
