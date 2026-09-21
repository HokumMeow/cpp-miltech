#pragma once

#include "failsafe/FailsafeFsm.h"

enum class NavAction {
    Manual,   // ручне керування failsafe не активний
    Hold,     // failsafe активний але без GPS
    Climb,    
    FlyHome,  
    Land      
};

const char* toString(NavAction a);

// інформація для автопілота
struct NavCommand {
    NavAction action = NavAction::Manual;
    double distanceToHomeM = 0.0;
    double bearingToHomeDeg = 0.0;  
    double headingErrorDeg = 0.0;   // на скільки повернути 
    double climbM = 0.0;            // на скільки набрати висоту
};

NavCommand computeNav(const FailsafeFsm& fsm, const Telemetry& t);
