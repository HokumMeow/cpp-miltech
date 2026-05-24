#include "ballistics.hpp"
#include <gtest/gtest.h>

TEST(Ballistics, ComputesKnownDropPoint)
{
  const BallisticsInput input{
    .drone_x_ = 100.0,
    .drone_y_ = 100.0,
    .drone_z_ = 100.0,
    .target_x_ = 200.0,
    .target_y_ = 200.0,
    .attack_speed_ = 10.0,
    .acceleration_path_ = 10.0,
    .ammo_name_ = "VOG-17",
  };
  const auto solution = compute_drop_solution(input);
  ASSERT_TRUE(solution);
  EXPECT_NEAR(solution->fire_x_, 173.759, 0.01);
  EXPECT_NEAR(solution->fire_y_, 173.759, 0.01);
}

TEST(Ballistics, HandlesUnknownAmmo)
{
  const BallisticsInput input{
    .drone_x_ = 100.0,
    .drone_y_ = 100.0,
    .drone_z_ = 100.0,
    .target_x_ = 200.0,
    .target_y_ = 200.0,
    .attack_speed_ = 10.0,
    .acceleration_path_ = 10.0,
    .ammo_name_ = "FAB-500",
  };
  EXPECT_FALSE(compute_drop_solution(input));
}

TEST(Ballistics, HandlesZdNegative)
{
  const BallisticsInput input{
    .drone_x_ = 100.0,
    .drone_y_ = 100.0,
    .drone_z_ = -100.0,
    .target_x_ = 200.0,
    .target_y_ = 200.0,
    .attack_speed_ = 10.0,
    .acceleration_path_ = 10.0,
    .ammo_name_ = "VOG-17",
  };
  EXPECT_FALSE(compute_drop_solution(input));
}