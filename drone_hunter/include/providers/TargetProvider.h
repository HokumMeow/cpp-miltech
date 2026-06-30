#pragma once
#include <vector>
#include <string>
#include "interfaces/ITargetProvider.h"
 
class JsonTargetProvider : public ITargetProvider {

public:
    JsonTargetProvider(const std::string& path, const float arrayTimeStep);
    int    getTargetCount() override { return targetCount_; }
    int    getTimeSteps() override { return timeSteps_; }
    Target getTarget(int idx) override { return current_.at(idx); }
    void   update(float time) override;
    Coord getPositionAt(int idx, float time) override;
    ~JsonTargetProvider();
private:
    std::vector<std::vector<Coord>> targets;
    int     targetCount_;
    int     timeSteps_;
    float   arrayTimeStep_;
    
    std::vector<Target> current_;
};