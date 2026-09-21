#pragma once

#include "common/Telemetry.h"

// джерело телеметрії - справжні датчики на малині або емулятор
class ITelemetrySource {
public:
    virtual ~ITelemetrySource() = default;
    
    virtual bool next(Telemetry& out) = 0;
};
