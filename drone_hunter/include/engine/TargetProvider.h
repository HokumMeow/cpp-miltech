#pragma once
#include "interfaces/ITargetProvider.h"
 
class JsonTargetProvider : public ITargetProvider {

public:
    JsonTargetProvider(const char* path, const float arrayTimeStep);
    int    getTargetCount() override { return targetCount_; }
    int    getTimeSteps() override { return timeSteps_; }
    Target getTarget(int idx) override { return current_[idx]; }
    void   update(float time) override;
    Coord getPositionAt(int idx, float time) override;
    ~JsonTargetProvider();
private:
    Coord** targets;
    int     targetCount_;
    int     timeSteps_;
    float   arrayTimeStep_;
    
    Target* current_ = nullptr;
};