#include "source/ReplayTelemetrySource.h"

#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace {

constexpr std::size_t kMinColumns = 7;
// t_ms,lat,lon,alt_m,heading_deg,link,sats,state,roll_deg,pitch_deg,temp_c
constexpr std::size_t kColumnsWithSensors = 11;

std::vector<std::string> splitCsv(const std::string& line) {
    std::vector<std::string> cols;
    std::istringstream ss(line);
    std::string cell;
    while (std::getline(ss, cell, ',')) cols.push_back(cell);
    return cols;
}

}  // namespace

ReplayTelemetrySource::ReplayTelemetrySource(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("could not open scenario file: " + path);
    load(in, path);
}

ReplayTelemetrySource::ReplayTelemetrySource(std::istream& in, const std::string& name) {
    load(in, name);
}

void ReplayTelemetrySource::load(std::istream& in, const std::string& name) {
    std::string line;
    int lineNo = 0;

    while (std::getline(in, line)) {
        ++lineNo;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#' || std::isalpha(static_cast<unsigned char>(line[0]))) continue;

        const std::string where = name + ":" + std::to_string(lineNo) + ": ";
        const std::vector<std::string> c = splitCsv(line);
        if (c.size() < kMinColumns) throw std::runtime_error(where + "expected at least 7 columns");

        Telemetry t;
        try {
            t.timeMs = std::stoll(c[0]);
            t.gps.latDeg = std::stod(c[1]);
            t.gps.lonDeg = std::stod(c[2]);
            t.baro.altitudeM = std::stod(c[3]);
            t.imu.headingDeg = std::stod(c[4]);
            t.linkOk = std::stoi(c[5]) != 0;
            t.gps.satellites = std::stoi(c[6]);
        } catch (const std::exception&) {
            throw std::runtime_error(where + "invalid number");
        }
        t.gps.valid = t.gps.satellites > 0;

        if (c.size() >= kColumnsWithSensors) {
            try {
                t.imu.rollDeg = std::stod(c[8]);
                t.imu.pitchDeg = std::stod(c[9]);
                t.baro.temperatureC = std::stod(c[10]);
            } catch (const std::exception&) {
                throw std::runtime_error(where + "invalid number");
            }
        }

        if (!rows_.empty() && t.timeMs <= rows_.back().timeMs) {
            throw std::runtime_error(where + "time must increase");
        }
        rows_.push_back(t);
    }

    if (rows_.empty()) throw std::runtime_error(name + ": no data rows");
}

bool ReplayTelemetrySource::next(Telemetry& out) {
    if (index_ >= rows_.size()) return false;
    out = rows_[index_++];
    return true;
}
