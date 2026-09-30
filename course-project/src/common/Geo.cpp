#include "common/Geo.h"

#include <cmath>

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDegToRad = kPi / 180.0;

void offsetM(const GeoPoint& a, const GeoPoint& b, double& east, double& north) {
    const double meanLat = (a.latDeg + b.latDeg) / 2.0;
    north = (b.latDeg - a.latDeg) * geo::kMetersPerDegLat;
    east = (b.lonDeg - a.lonDeg) * geo::kMetersPerDegLat * std::cos(meanLat * kDegToRad);
}

}  // namespace

namespace geo {

double distanceM(const GeoPoint& a, const GeoPoint& b) {
    double east = 0.0;
    double north = 0.0;
    offsetM(a, b, east, north);
    return std::hypot(east, north);
}

double bearingDeg(const GeoPoint& from, const GeoPoint& to) {
    double east = 0.0;
    double north = 0.0;
    offsetM(from, to, east, north);
    return normalize360(std::atan2(east, north) / kDegToRad);
}

double normalize360(double deg) {
    double r = std::fmod(deg, 360.0);
    if (r < 0.0) r += 360.0;
    return r;
}

double normalize180(double deg) {
    double r = normalize360(deg);
    if (r > 180.0) r -= 360.0;
    return r;
}

}  // namespace geo
