#include "common/Attitude.h"

#include <cmath>

#include "common/Geo.h"

namespace {
constexpr double kRadToDeg = 180.0 / 3.14159265358979323846;
}

namespace attitude {

double rollDeg(double ay, double az) {
    return std::atan2(ay, az) * kRadToDeg;
}

double pitchDeg(double ax, double ay, double az) {
    // вісь що дивиться в небо дає +1 g
    // тому при піднятому носі додає ax
    return std::atan2(ax, std::hypot(ay, az)) * kRadToDeg;
}

double headingDeg(double magX, double magY, double offsetDeg) {
    // дивимось на схід , курс має бути 90
    return geo::normalize360(std::atan2(magX, magY) * kRadToDeg + offsetDeg);
}

}  // namespace attitude
