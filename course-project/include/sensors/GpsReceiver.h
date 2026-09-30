#pragma once
#include <cstdint>
#include <string>

#include "common/Telemetry.h"
#include "sensors/NmeaParser.h"

namespace gps {

// модуль шле RMC раз на секунду, якщо більше 2, то інформація застаріла
constexpr int64_t kFixTimeoutMs = 2000;

}  // namespace gps

class GpsReceiver {
public:
    explicit GpsReceiver(const std::string& port, int baud = 9600);
    ~GpsReceiver();

    GpsReceiver(const GpsReceiver&) = delete;
    GpsReceiver& operator=(const GpsReceiver&) = delete;

    void poll(int64_t nowMs);

    // якщо немає даних valid = false.
    GpsFix fix(int64_t nowMs) const;

private:
    int fd_;
    std::string lineBuffer_;
    NmeaParser parser_;
    int64_t lastFixMs_ = 0;
    bool haveFix_ = false;
};
