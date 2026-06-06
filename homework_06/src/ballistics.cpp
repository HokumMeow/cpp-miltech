#include <cmath>
#include <cstring>
#include <iostream>
#include <optional>

#include "ballistics.hpp"

constexpr float kGravity = 9.81F;

struct AmmoParams {
  float mass;
  float drag;
  float lift;
};

// NOLINTBEGIN(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)
auto calc_t(const AmmoParams& a, float attack_speed, float z_drone) -> float
{
  float drag = a.drag;
  float mass = a.mass;
  float lift = a.lift;
  float a_val = drag * kGravity * mass - 2.0F * drag * drag * lift * attack_speed;
  float b_val = -3.0F * kGravity * mass * mass + 3.0F * drag * lift * mass * attack_speed;
  float c_val = 6.0F * mass * mass * z_drone;
  float p_val = -(b_val * b_val) / (3.0F * a_val * a_val);
  float q_val = 2.0F * b_val * b_val * b_val / (27.0F * a_val * a_val * a_val) + c_val / a_val;
  double acos_arg = 3.0 * q_val / (2.0 * p_val) * std::sqrt(-3.0 / p_val);
  if (acos_arg < -1.0 || acos_arg > 1.0) {
    std::cerr << "phi out of range\n";
    return -1;
  }
  double phi = std::acos(acos_arg);
  double t_val = 2.0 * std::sqrt(-p_val / 3.0) * std::cos((phi + 4.0 * M_PI) / 3.0) - b_val / (3.0 * a_val);
  if (t_val <= 0) {
    return -1;
  }
  return static_cast<float>(t_val);
}

auto calc_h(const AmmoParams& a, float attack_speed, float t) -> float
{
  float t2 = t * t, t3 = t2 * t, t4 = t3 * t, t5 = t4 * t;
  float d2 = a.drag * a.drag, d3 = d2 * a.drag, d4 = d3 * a.drag;
  float l2 = a.lift * a.lift, l3 = l2 * a.lift, l4 = l3 * a.lift;
  float m2 = a.mass * a.mass, m3 = m2 * a.mass, m4 = m3 * a.mass;
  return attack_speed * t - t2 * a.drag * attack_speed / (2.0F * a.mass) +
         t3 * (6.0F * a.drag * kGravity * a.lift * a.mass - 6.0F * d2 * (l2 - 1.0F) * attack_speed) / (36.0F * m2) +
         t4 *
           (-6.0F * d2 * kGravity * a.lift * (1.0F + l2 + l4) * a.mass + 3.0F * d3 * l2 * (1.0F + l2) * attack_speed +
            6.0F * d2 * a.drag * l4 * (1.0F + l2) * attack_speed) /
           (36.0F * (1.0F + l2) * (1.0F + l2) * m3) +
         t5 * (3.0F * d3 * kGravity * l3 * a.mass - 3.0F * d4 * l2 * (1.0F + l2) * attack_speed) / (36.0F * (1.0F + l2) * m4);
}
// NOLINTEND(readability-magic-numbers, cppcoreguidelines-avoid-magic-numbers)

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
static auto lookup_ammo(const char* name) -> std::optional<AmmoParams>
{
  if (strcmp(name, "VOG-17") == 0)
    return AmmoParams{0.35F, 0.07F, 0.0F};
  if (strcmp(name, "M67") == 0)
    return AmmoParams{0.6F, 0.1F, 0.0F};
  if (strcmp(name, "RKG-3") == 0)
    return AmmoParams{1.2F, 0.1F, 0.0F};
  if (strcmp(name, "GLIDING-VOG") == 0)
    return AmmoParams{0.45F, 0.1F, 1.0F};
  if (strcmp(name, "GLIDING-RKG") == 0)
    return AmmoParams{1.4F, 0.1F, 1.0F};
  return std::nullopt;
}
// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)

auto compute_drop_solution(const BallisticsInput& input) -> std::optional<DropSolution>
{
  auto ammo_opt = lookup_ammo(input.ammo_name_);
  if (!ammo_opt)
    return std::nullopt;
  const AmmoParams& ammo = *ammo_opt;

  auto t_val = calc_t(ammo, input.attack_speed_, input.drone_z_);
  if (t_val <= 0) {
    std::cerr << "t out of range\n";
    return std::nullopt;
  }

  auto h_val = calc_h(ammo, input.attack_speed_, t_val);

  // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)
  float d_val = std::sqrt(std::pow(input.target_x_ - input.drone_x_, 2.0F) + std::pow(input.target_y_ - input.drone_y_, 2.0F));
  // NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
  if (d_val <= 0) {
    std::cerr << "D out of range\n";
    return std::nullopt;
  }

  DropSolution solution;
  float ratio = (d_val - h_val) / d_val;
  solution.fire_x_ = input.drone_x_ + (input.target_x_ - input.drone_x_) * ratio;
  solution.fire_y_ = input.drone_y_ + (input.target_y_ - input.drone_y_) * ratio;

  if (h_val + input.acceleration_path_ > d_val) {
    solution.intermediate_x_ = input.target_x_ - (input.target_x_ - input.drone_x_) * (h_val + input.acceleration_path_) / d_val;
    solution.intermediate_y_ = input.target_y_ - (input.target_y_ - input.drone_y_) * (h_val + input.acceleration_path_) / d_val;
  }

  return solution;
}
