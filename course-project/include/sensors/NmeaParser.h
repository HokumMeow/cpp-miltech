#pragma once
#include <string>
#include <vector>

#include "common/Telemetry.h"

// парсінг NMEA GPS
class NmeaParser {
public:
    bool parseLine(const std::string& line);

    const GpsFix& fix() const { return fix_; }

private:
    bool parseRmc(const std::vector<std::string>& f);
    bool parseGga(const std::vector<std::string>& f);

    GpsFix fix_;
};
