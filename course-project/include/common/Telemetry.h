#pragma once
#include <cstdint>

// Дані GPS, розібр з NMEA
struct GpsFix {
    bool valid = false;
    double latDeg = 0.0;
    double lonDeg = 0.0;
    int satellites = 0;
    double speedMps = 0.0;
};

// Орієнтація з GY-9250
struct ImuSample {
    double rollDeg = 0.0;
    double pitchDeg = 0.0;
    double headingDeg = 0.0;
};

// Дані барометра BMP388.
struct BaroSample {
    double pressurePa = 0.0;
    double temperatureC = 0.0;
    double altitudeM = 0.0;
};

struct Telemetry {
    int64_t timeMs = 0;
    GpsFix gps;
    ImuSample imu;
    BaroSample baro;
    bool linkOk = true; 
};
