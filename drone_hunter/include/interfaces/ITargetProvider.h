#pragma once
#include "dto/Target.h"

class ITargetProvider {
public:
    virtual int getTargetCount() = 0;
    virtual Target getTarget(int idx) = 0;
    virtual void update(float time) {}
    virtual ~ITargetProvider() {}
};
