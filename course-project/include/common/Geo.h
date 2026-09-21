#pragma once

struct GeoPoint {
    double latDeg = 0.0;
    double lonDeg = 0.0;
};

namespace geo {

constexpr double kMetersPerDegLat = 111320.0;

// Відстань, метри
double distanceM(const GeoPoint& a, const GeoPoint& b);

// Азимут 0-360 градусів, 0 = північ
double bearingDeg(const GeoPoint& from, const GeoPoint& to);

double normalize360(double deg); 
double normalize180(double deg); 

}  // namespace geo
