// ============================================================================
// ТЕСТИ: географія
// ============================================================================
// Перевіряємо конкретні числа, які можна порахувати на папері й перевірити по карті:
//   * 0.001 градуса широти - це 111.32 м (меридіан скрізь однаковий);
//   * на широті 60 градусів довгота вдвічі "коротша", бо cos(60) = 0.5;
//   * чотири сторони світу дають рівно 0, 90, 180, 270.
// ============================================================================

#include <gtest/gtest.h>

#include "common/Geo.h"

TEST(Geo, OneThousandthDegreeOfLatitudeIs111Meters) {
    const GeoPoint a{50.0, 30.0};
    const GeoPoint b{50.001, 30.0};
    EXPECT_NEAR(geo::distanceM(a, b), 111.32, 0.01);
}

TEST(Geo, LongitudeIsShorterThanLatitude) {
    const GeoPoint a{60.0, 30.0};
    const GeoPoint b{60.0, 30.001};
    // на широті 60 градусів косинус дорівнює 0.5
    EXPECT_NEAR(geo::distanceM(a, b), 55.66, 0.05);
}

TEST(Geo, DistanceToSelfIsZero) {
    const GeoPoint a{50.4501, 30.5234};
    EXPECT_DOUBLE_EQ(geo::distanceM(a, a), 0.0);
}

TEST(Geo, BearingToCardinalDirections) {
    const GeoPoint o{50.0, 30.0};
    EXPECT_NEAR(geo::bearingDeg(o, {50.001, 30.0}), 0.0, 0.01);     // північ
    EXPECT_NEAR(geo::bearingDeg(o, {50.0, 30.001}), 90.0, 0.01);    // схід
    EXPECT_NEAR(geo::bearingDeg(o, {49.999, 30.0}), 180.0, 0.01);   // південь
    EXPECT_NEAR(geo::bearingDeg(o, {50.0, 29.999}), 270.0, 0.01);   // захід
}

TEST(Geo, Normalize360) {
    EXPECT_DOUBLE_EQ(geo::normalize360(370.0), 10.0);
    EXPECT_DOUBLE_EQ(geo::normalize360(-90.0), 270.0);
    EXPECT_DOUBLE_EQ(geo::normalize360(0.0), 0.0);
}

TEST(Geo, Normalize180) {
    EXPECT_DOUBLE_EQ(geo::normalize180(190.0), -170.0);
    EXPECT_DOUBLE_EQ(geo::normalize180(-190.0), 170.0);
    EXPECT_DOUBLE_EQ(geo::normalize180(90.0), 90.0);
}
