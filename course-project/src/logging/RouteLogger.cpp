#include "logging/RouteLogger.h"

#include <algorithm>
#include <cstdio>
#include <stdexcept>

RouteLogger::RouteLogger(const std::string& path) {
    if (path.empty()) return;

    file_.open(path);
    if (!file_) throw std::runtime_error("could not open route log for writing: " + path);
    file_ << "t_ms,lat,lon,alt_m,heading_deg,link,sats,state,roll_deg,pitch_deg,temp_c\n";
}

void RouteLogger::setHome(const GeoPoint& home) {
    home_ = home;
    haveHome_ = true;
}

void RouteLogger::log(const Telemetry& t, FlightState state) {
    ++stats_.points;

    if (t.gps.valid) {
        const GeoPoint pos{t.gps.latDeg, t.gps.lonDeg};
        if (havePrev_) stats_.pathLengthM += geo::distanceM(prev_, pos);
        if (haveHome_) stats_.maxDistanceFromHomeM = std::max(stats_.maxDistanceFromHomeM, geo::distanceM(home_, pos));
        prev_ = pos;
        havePrev_ = true;
    }

    if (!file_.is_open()) return;

    char line[224];
    std::snprintf(line, sizeof line, "%lld,%.7f,%.7f,%.2f,%.1f,%d,%d,%s,%.1f,%.1f,%.1f\n",
                  static_cast<long long>(t.timeMs), t.gps.latDeg, t.gps.lonDeg, t.baro.altitudeM,
                  t.imu.headingDeg, t.linkOk ? 1 : 0, t.gps.valid ? t.gps.satellites : 0, toString(state),
                  t.imu.rollDeg, t.imu.pitchDeg, t.baro.temperatureC);
    file_ << line << std::flush;
}
