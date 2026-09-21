// ============================================================================
// ТЕСТИ: орієнтація
// ============================================================================
// Вхідні дані тут що реально покаже чип у певній позі
//
// Тести курсу перевіряють усі чотири сторони світу - це ловить і переставлені
// осі магнітометра, і дзеркальний напрямок відліку.
// ============================================================================

#include <gtest/gtest.h>

#include "common/Attitude.h"

TEST(Attitude, FlatBoardHasZeroRollAndPitch) {
    EXPECT_NEAR(attitude::rollDeg(0.0, 1.0), 0.0, 1e-9);
    EXPECT_NEAR(attitude::pitchDeg(0.0, 0.0, 1.0), 0.0, 1e-9);
}

// Акселерометр показує +1 g уздовж тієї осі, яка дивиться в небо. Тому при нахилі
// праворуч піднімається лівий борт (вісь Y) і ay стає додатним, а при піднятому носі
// в небо дивиться вісь X і додатним стає ax.
TEST(Attitude, RollRightGivesPositiveRoll) {
    // нахил на 45 градусів: гравітація ділиться порівну між Y і Z
    EXPECT_NEAR(attitude::rollDeg(0.7071, 0.7071), 45.0, 0.01);
}

TEST(Attitude, RollLeftGivesNegativeRoll) {
    EXPECT_NEAR(attitude::rollDeg(-0.7071, 0.7071), -45.0, 0.01);
}

TEST(Attitude, NoseUpGivesPositivePitch) {
    EXPECT_NEAR(attitude::pitchDeg(0.7071, 0.0, 0.7071), 45.0, 0.01);
    EXPECT_NEAR(attitude::pitchDeg(1.0, 0.0, 0.0), 90.0, 0.01);  // ніс строго вгору
}

TEST(Attitude, NoseDownGivesNegativePitch) {
    EXPECT_NEAR(attitude::pitchDeg(-0.7071, 0.0, 0.7071), -45.0, 0.01);
}

TEST(Attitude, HeadingFromMagnetometer) {
    // Вісь Y магнітометра дивиться на ніс, X - ліворуч.
    EXPECT_NEAR(attitude::headingDeg(0.0, 20.0), 0.0, 1e-9);     // північ спереду
    EXPECT_NEAR(attitude::headingDeg(20.0, 0.0), 90.0, 1e-9);    // північ ліворуч: ніс на схід
    EXPECT_NEAR(attitude::headingDeg(0.0, -20.0), 180.0, 1e-9);  // північ ззаду: ніс на південь
    EXPECT_NEAR(attitude::headingDeg(-20.0, 0.0), 270.0, 1e-9);  // північ праворуч: ніс на захід
}

TEST(Attitude, HeadingOffsetIsAppliedAndWrapped) {
    EXPECT_NEAR(attitude::headingDeg(0.0, 20.0, 90.0), 90.0, 1e-9);
    EXPECT_NEAR(attitude::headingDeg(-20.0, 0.0, 180.0), 90.0, 1e-9);
}
