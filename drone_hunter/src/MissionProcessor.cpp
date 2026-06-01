#include "core/MissionProcessor.h"


std::optional<DropPoint> MissionProcessor::step() {
        Target t = targets_->getTarget(currentIdx_++);
        return solver_->solve(dronePos_, t.pos, altitude_, ammo_, t.velocity);
        
        

}

    void changeSolver(IBallisticSolver* s) { solver_ = s; }  // ← підміна на льоту


    //auto* prov = createProvider(
    //    SourceType::JSON, "targets.json");
    //delete prov;

 
};