#define USE_MATH_DEFINES
#include <iostream>
#include <cmath>
#include <cstring>
#include <optional>
#include "ballistics.hpp"

//using namespace std;

constexpr float kGravity = 9.81F;
constexpr float kFloatDigit1 = 1.0F;
constexpr float kFloatDigit2 = 2.0F;
constexpr float kFloatDigit3 = 3.0F;
constexpr float kFloatDigit6 = 6.0F;
constexpr float kFloatDigit27 = 27.0F;
constexpr float kFloatDigit36 = 36.0F;

struct AmmoParams {
  float mass;
  float drag;
  float lift;
};

auto calc_t(const AmmoParams& a, float attack_speed, float z_drone) -> float
{
  float drag = a.drag;
  float mass = a.mass;
  float lift = a.lift;
  float a_val = drag * kGravity * mass - kFloatDigit2 * drag * drag * lift * attack_speed;
  float b_val = -kFloatDigit3 * kGravity * mass * mass + kFloatDigit3 * drag * lift * mass * attack_speed;
  float c_val = kFloatDigit6 * mass * mass * z_drone;
  float p_val = (-(b_val * b_val)) / (kFloatDigit3 * a_val * a_val);
  float q_val = kFloatDigit2 * b_val * b_val * b_val / (kFloatDigit27 * a_val * a_val * a_val) + c_val / a_val;
  double acosArg = kFloatDigit3 * q_val / (kFloatDigit2 * p_val) * std::sqrt(-kFloatDigit3 / p_val);
  if (acosArg < -1.0 || acosArg > 1.0) {
    std::cerr << "phi out of range\n";
    return -1;
  }
  double phi = std::acos(acosArg);
  constexpr double kDoubleDigit3 = 3.0;
  constexpr double kDoubleDigit4 = 4.0;
  double t_val = kFloatDigit2 * std::sqrt(-p_val / kFloatDigit3) * std::cos((phi + kDoubleDigit4 * M_PI) / kFloatDigit3) - b_val / (kDoubleDigit3 * a_val);
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
  float result = attack_speed * t - t2 * a.drag * attack_speed / (kFloatDigit2 * a.mass) +
                 t3 * (kFloatDigit6 * a.drag * kGravity * a.lift * a.mass - kFloatDigit6 * d2 * (l2 - kFloatDigit1) * attack_speed) / (kFloatDigit36 * m2) +
                 t4 *
                   (-kFloatDigit6 * d2 * kGravity * a.lift * (kFloatDigit1 + l2 + l4) * a.mass + kFloatDigit3 * d3 * l2 * (kFloatDigit1 + l2) * attack_speed +
                    kFloatDigit6 * d2 * a.drag * l4 * (kFloatDigit1 + l2) * attack_speed) /
                   (kFloatDigit36 * (kFloatDigit1 + l2) * (kFloatDigit1 + l2) * m3) +
                 t5 * (kFloatDigit3 * d3 * kGravity * l3 * a.mass - kFloatDigit3 * d4 * l2 * (kFloatDigit1 + l2) * attack_speed) / (kFloatDigit36 * (kFloatDigit1 + l2) * m4);
  return result;
}

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers) для зручності читання формул, оскільки вони є частиною фізичних розрахунків і їх складно замінити на константи з описовими іменами без втрати зрозумілості формул.
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

  float d_val = std::sqrt(std::pow(input.target_x_ - input.drone_x_, kFloatDigit2) + std::pow(input.target_y_ - input.drone_y_, kFloatDigit2));
  if (d_val <= 0) {
    std::cerr << "D out of range\n";
    return std::nullopt;
  }

  DropSolution solution;
  float ratio = (d_val - h_val) / d_val;
  solution.fire_x_ = input.drone_x_ + (input.target_x_ - input.drone_x_) * ratio;
  solution.fire_y_ = input.drone_y_ + (input.target_y_ - input.drone_y_) * ratio;

  if (h_val + input.acceleration_path_ > d_val) {
    float x_drone = input.target_x_ - (input.target_x_ - input.drone_x_) * (h_val + input.acceleration_path_) / d_val;
    float y_drone = input.target_y_ - (input.target_y_ - input.drone_y_) * (h_val + input.acceleration_path_) / d_val;

    solution.fire_x_ = x_drone;
    solution.fire_y_ = y_drone;
    return std::optional<DropSolution>{solution};
  }

  return std::optional<DropSolution>{solution};
}