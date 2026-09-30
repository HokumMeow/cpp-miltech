#pragma once
#include <cstdint>

#include "common/Geo.h"
#include "common/Telemetry.h"

enum class FlightState {
    WaitingForFix,  // GPS щоб запам'ятати хоум поінт
    Normal,         // є зв'язок
    LinkLost,       // зв'язку нема
    Climb,          // failsafe набір безпечної висоти
    Return,         // failsafe політ додому
    Home            
};

const char* toString(FlightState s);

struct FailsafeConfig {
    int minSatellites = 4;          // мінімум супутників для хоум поінт
    int64_t linkTimeoutMs = 3000;   // таймаут до старту RTH
    double rthAltitudeM = 30.0;     // умовно безпечна висота для RTH
    double altToleranceM = 2.0;     // похтибка висоти при наборі висоти
    double homeRadiusM = 3.0;       // похибка координат дому

    // false - для перевірки в приміщенні, бо координат нема
    bool requireGpsForHome = true;
};

// Автомат станів failsafe
// час з Telemetry.timeMs щоб всюди був однаковий час
class FailsafeFsm {
public:
    explicit FailsafeFsm(FailsafeConfig cfg = {});

    FlightState update(const Telemetry& t);

    FlightState state() const { return state_; }
    bool hasHome() const { return haveHome_; }                    
    bool homeHasPosition() const { return homeHasPosition_; }     
    const GeoPoint& home() const { return home_; }
    double homeAltitudeM() const { return homeAltM_; }
    const FailsafeConfig& config() const { return cfg_; }

private:
    FailsafeConfig cfg_;
    FlightState state_ = FlightState::WaitingForFix;
    bool haveHome_ = false;
    bool homeHasPosition_ = false;
    GeoPoint home_;
    double homeAltM_ = 0.0;
    int64_t linkLostSinceMs_ = 0;
};
