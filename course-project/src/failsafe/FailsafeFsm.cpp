#include "failsafe/FailsafeFsm.h"

const char* toString(FlightState s) {
    switch (s) {
        case FlightState::WaitingForFix: return "WAIT_FIX";
        case FlightState::Normal: return "NORMAL";
        case FlightState::LinkLost: return "LINK_LOST";
        case FlightState::Climb: return "CLIMB";
        case FlightState::Return: return "RETURN";
        case FlightState::Home: return "HOME";
    }
    return "?";
}

FailsafeFsm::FailsafeFsm(FailsafeConfig cfg) : cfg_(cfg) {}

FlightState FailsafeFsm::update(const Telemetry& t) {
    const bool gpsGood = t.gps.valid && t.gps.satellites >= cfg_.minSatellites;
    const GeoPoint pos{t.gps.latDeg, t.gps.lonDeg};

    switch (state_) {
        case FlightState::WaitingForFix:
            if (gpsGood) {
                home_ = pos;
                homeAltM_ = t.baro.altitudeM;
                haveHome_ = true;
                homeHasPosition_ = true;
                state_ = FlightState::Normal;
            } else if (!cfg_.requireGpsForHome) {
                // перевірка без GPS, координат нема
                homeAltM_ = t.baro.altitudeM;
                haveHome_ = true;
                state_ = FlightState::Normal;
            }
            break;

        case FlightState::Normal:
            if (!t.linkOk) {
                linkLostSinceMs_ = t.timeMs;
                state_ = FlightState::LinkLost;
            }
            break;

        case FlightState::LinkLost:
            if (t.linkOk) {
                state_ = FlightState::Normal;
            } else if (t.timeMs - linkLostSinceMs_ >= cfg_.linkTimeoutMs) {
                state_ = FlightState::Climb;
            }
            break;

        case FlightState::Climb: {
            const double relAlt = t.baro.altitudeM - homeAltM_;
            if (relAlt >= cfg_.rthAltitudeM - cfg_.altToleranceM) {
                state_ = FlightState::Return;
            }
            break;
        }

        case FlightState::Return:
            // якщо немає координат дому - лишаємось у Return
            if (homeHasPosition_ && t.gps.valid && geo::distanceM(pos, home_) <= cfg_.homeRadiusM) {
                state_ = FlightState::Home;
            }
            break;

        case FlightState::Home:
            break;
    }
    return state_;
}
