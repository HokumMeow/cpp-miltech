#pragma once
#include <fstream>
#include <string>

#include "common/Geo.h"
#include "common/Telemetry.h"
#include "failsafe/FailsafeFsm.h"

// пише маршрут у CSV 
// t_ms,lat,lon,alt_m,heading_deg,link,sats,state,roll_deg,pitch_deg,temp_c

class RouteLogger {
public:
    struct Stats {
        int points = 0;
        double pathLengthM = 0.0;
        double maxDistanceFromHomeM = 0.0;
    };

    explicit RouteLogger(const std::string& path = "");

    void setHome(const GeoPoint& home);
    void log(const Telemetry& t, FlightState state);

    const Stats& stats() const { return stats_; }

private:
    std::ofstream file_;
    Stats stats_;
    bool haveHome_ = false;
    GeoPoint home_;
    bool havePrev_ = false;
    GeoPoint prev_;
};
