#pragma once
#include <istream>
#include <string>
#include <vector>

#include "source/ITelemetrySource.h"

// відтворює телеметрію з CSV яку пише RouteLogger
// t_ms,lat,lon,alt_m,heading_deg,link,sats[,state,roll_deg,pitch_deg,temp_c]
class ReplayTelemetrySource : public ITelemetrySource {
public:
    explicit ReplayTelemetrySource(const std::string& path);
    ReplayTelemetrySource(std::istream& in, const std::string& name);

    bool next(Telemetry& out) override;

    std::size_t size() const { return rows_.size(); }

private:
    void load(std::istream& in, const std::string& name);

    std::vector<Telemetry> rows_;
    std::size_t index_ = 0;
};
