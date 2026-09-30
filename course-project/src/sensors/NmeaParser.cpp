#include "sensors/NmeaParser.h"

#include <cmath>
#include <cstdlib>

namespace {

constexpr double kKnotsToMps = 0.514444;

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> parts;
    std::string current;
    for (char c : s) {
        if (c == sep) {
            parts.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    parts.push_back(current);
    return parts;
}

bool toDouble(const std::string& s, double& out) {
    if (s.empty()) return false;
    char* end = nullptr;
    out = std::strtod(s.c_str(), &end);
    return end == s.c_str() + s.size();
}

bool toDegrees(const std::string& value, const std::string& hemisphere, double& out) {
    double raw = 0.0;
    if (!toDouble(value, raw) || hemisphere.size() != 1) return false;
    const double degrees = std::floor(raw / 100.0);
    const double minutes = raw - degrees * 100.0;
    out = degrees + minutes / 60.0;
    if (hemisphere == "S" || hemisphere == "W") out = -out;
    return true;
}

bool checksumOk(const std::string& line, std::size_t star) {
    unsigned calc = 0;
    for (std::size_t i = 1; i < star; ++i) calc ^= static_cast<unsigned char>(line[i]);

    char* end = nullptr;
    const std::string hex = line.substr(star + 1, 2);
    const unsigned given = static_cast<unsigned>(std::strtoul(hex.c_str(), &end, 16));
    return end == hex.c_str() + hex.size() && calc == given;
}

}  // namespace

bool NmeaParser::parseLine(const std::string& line) {
    if (line.size() < 9 || line[0] != '$') return false;

    const std::size_t star = line.find('*');
    if (star == std::string::npos || star + 3 > line.size()) return false;
    if (!checksumOk(line, star)) return false;

    const std::vector<std::string> fields = split(line.substr(1, star - 1), ',');
    const std::string& id = fields[0];
    if (id.size() < 5) return false;

    const std::string type = id.substr(id.size() - 3);
    if (type == "RMC") return parseRmc(fields);
    if (type == "GGA") return parseGga(fields);
    return false;
}

bool NmeaParser::parseRmc(const std::vector<std::string>& f) {
    if (f.size() < 8) return false;

    if (f[2] != "A") {  // немає фіксації
        fix_.valid = false;
        return true;
    }

    double lat = 0.0;
    double lon = 0.0;
    double knots = 0.0;
    if (!toDegrees(f[3], f[4], lat) || !toDegrees(f[5], f[6], lon) || !toDouble(f[7], knots)) {
        fix_.valid = false;
        return false;
    }

    fix_.valid = true;
    fix_.latDeg = lat;
    fix_.lonDeg = lon;
    fix_.speedMps = knots * kKnotsToMps;
    return true;
}

bool NmeaParser::parseGga(const std::vector<std::string>& f) {
    if (f.size() < 8) return false;

    double sats = 0.0;
    fix_.satellites = toDouble(f[7], sats) ? static_cast<int>(sats) : 0;
    return true;
}
