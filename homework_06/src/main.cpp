#include <iostream>
#include <fstream>
#include <span>
#include <string>

#include "ballistics.hpp"

auto main(int argc, char* argv[]) -> int
{
  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic) для зручності обробки аргументів командного рядка за допомогою
  // std::span, оскільки це дозволяє легко отримати розмір і доступ до аргументів без необхідності ручного керування вказівниками.
  const auto kArgs = std::span<char*>(argv, static_cast<std::size_t>(argc));
  if (kArgs.size() < 3) {
    std::cerr << "usage: ballistic <input_path> <output_path>\n";
    return 1;
  }

  BallisticsInput input = {};

  std::string ammo_name;

  std::ifstream file_input(kArgs[1]);
  if (!file_input.is_open()) {
    std::cerr << "Error opening file\n";
    return 1;
  }

  file_input >> input.drone_x_ >> input.drone_y_ >> input.drone_z_ >> input.target_x_ >> input.target_y_ >> input.attack_speed_ >>
    input.acceleration_path_ >> ammo_name;
  input.ammo_name_ = ammo_name.data();
  file_input.close();

  auto solution_opt = compute_drop_solution(input);
  if (!solution_opt) {
    std::cerr << "No drop solution found\n";
    return 1;
  }
  DropSolution solution = *solution_opt;

  std::ofstream output_file(kArgs[2]);
  if (!output_file.is_open()) {
    std::cerr << "Error opening output file\n";
    return 1;
  }

  if (solution.intermediate_x_ != 0.F && solution.intermediate_y_ != 0.F) {
    output_file << solution.intermediate_x_ << " " << solution.intermediate_y_ << " ";
  }

  output_file << solution.fire_x_ << " " << solution.fire_y_ << "\n";
  output_file.close();

  return 0;
}