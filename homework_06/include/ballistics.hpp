#pragma once
#include <optional>

struct BallisticsInput {
  float drone_x_, drone_y_, drone_z_;
  float target_x_, target_y_;
  float attack_speed_, acceleration_path_;
  const char* ammo_name_;
};

struct DropSolution {
  float fire_x_ = 0.F, fire_y_ = 0.F, intermediate_x_ = 0.F, intermediate_y_ = 0.F;
};

auto compute_drop_solution(const BallisticsInput& input) -> std::optional<DropSolution>;
