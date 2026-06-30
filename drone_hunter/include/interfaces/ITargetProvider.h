#pragma once
#include "dto/Target.h"
#include "interfaces/IThreadedComponent.h"

class ITargetProvider : public IThreadedComponent {
public:
    virtual int getTargetCount() const = 0;
    virtual Target getTarget(int idx) const = 0;
    virtual ~ITargetProvider() override {}
};
