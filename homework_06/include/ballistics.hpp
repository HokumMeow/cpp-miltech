struct BallisticsInput {
    float drone_x, drone_y, drone_z;
    float target_x, target_y;
    float attack_speed, acceleration_path;
    const char* ammo_name;
};

struct DropSolution {
    float fire_x = 0.f, fire_y = 0.f, intermediate_x = 0.f, intermediate_y = 0.f;
};

DropSolution compute_drop_solution(const BallisticsInput& input);