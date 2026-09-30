#include "nav/Navigator.h"

const char* toString(NavAction a) {
    switch (a) {
        case NavAction::Manual: return "MANUAL";
        case NavAction::Hold: return "HOLD";
        case NavAction::Climb: return "CLIMB";
        case NavAction::FlyHome: return "FLY_HOME";
        case NavAction::Land: return "LAND";
    }
    return "?";
}

NavCommand computeNav(const FailsafeFsm& fsm, const Telemetry& t) {
    NavCommand cmd;
    if (!fsm.hasHome()) return cmd;

    const double relAlt = t.baro.altitudeM - fsm.homeAltitudeM();
    cmd.climbM = fsm.config().rthAltitudeM - relAlt;

    const bool canNavigate = t.gps.valid && fsm.homeHasPosition();
    if (canNavigate) {
        const GeoPoint pos{t.gps.latDeg, t.gps.lonDeg};
        cmd.distanceToHomeM = geo::distanceM(pos, fsm.home());
        cmd.bearingToHomeDeg = geo::bearingDeg(pos, fsm.home());
        cmd.headingErrorDeg = geo::normalize180(cmd.bearingToHomeDeg - t.imu.headingDeg);
    }

    switch (fsm.state()) {
        case FlightState::Climb:
            cmd.action = NavAction::Climb;
            break;
        case FlightState::Return:
            cmd.action = canNavigate ? NavAction::FlyHome : NavAction::Hold;
            break;
        case FlightState::Home:
            cmd.action = NavAction::Land;
            break;
        default:
            cmd.action = NavAction::Manual;
            break;
    }
    return cmd;
}
