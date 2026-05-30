#include "core/MissionProcessor.h"

class MissionProcessor {
    IBallisticSolver* solver_;   
    ITargetProvider* targets_;

    // ...
public:
    DropPoint step() {
        Target t = targets_->getTarget(currentIdx_++);
        return solver_->solve(dronePos_, t.pos, altitude_, ammo_);
        
        

    }

    void changeSolver(IBallisticSolver* s) { solver_ = s; }  // ← підміна на льоту


    //auto* prov = createProvider(
    //    SourceType::JSON, "targets.json");
    //delete prov;

 
};