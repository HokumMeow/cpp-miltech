class MissionProcessor {
    IBallisticSolver* solver_;   // ← вказівник на ІНТЕРФЕЙС, не на AnalyticalSolver
    ITargetProvider* targets_;
    // ...
public:
    DropPoint step() {
        Target t = targets_->getTarget(currentIdx_++);
        return solver_->solve(dronePos_, t.pos, altitude_, ammo_);
        //     ^^^^^^^ виклик через інтерфейс — НЕ знає, аналітика це чи таблиця
    }

    void changeSolver(IBallisticSolver* s) { solver_ = s; }  // ← підміна на льоту


 
};