#include <gtest/gtest.h>

#include "antidrone_turret/turret_decision.hpp"

namespace {

using antidrone_turret::TargetObservation;
using antidrone_turret::TargetState;
using antidrone_turret::TriggerDecision;
using antidrone_turret::TurretAction;
using antidrone_turret::TurretDecisionConfig;

TurretDecisionConfig default_config()
{
  return TurretDecisionConfig{0.80F, 30.0F};
}

TEST(TurretDecisionTest, NotVisibleIsTargetNone)
{
  const auto target = TargetObservation{false, 320.0F, 240.0F, 10.0F, 0.95F};

  const auto decision = antidrone_turret::decide(target, true, default_config());

  EXPECT_EQ(decision.status.target_state, TargetState::kNone);
  EXPECT_EQ(decision.status.action, TurretAction::kIdle);
  EXPECT_EQ(decision.status.trigger_state, TriggerDecision::kSkip);
  EXPECT_FALSE(decision.should_publish_aim_commands);
  EXPECT_FALSE(decision.should_request_trigger);
}

TEST(TurretDecisionTest, LowConfidenceIsActionIdleAndTriggerSkip)
{
  const auto target = TargetObservation{true, 420.0F, 180.0F, 15.0F, 0.60F};

  const auto decision = antidrone_turret::decide(target, true, default_config());

  EXPECT_EQ(decision.status.target_state, TargetState::kLowConfidence);
  EXPECT_EQ(decision.status.action, TurretAction::kIdle);
  EXPECT_EQ(decision.status.trigger_state, TriggerDecision::kSkip);
  EXPECT_FALSE(decision.should_publish_aim_commands);
}

TEST(TurretDecisionTest, TargetRightOfCenterProducesServoRight)
{
  const auto command = antidrone_turret::compute_servo_command(
    TargetObservation{true, 420.0F, 240.0F, 25.0F, 0.90F});

  EXPECT_EQ(command.direction, 1);
  EXPECT_GT(command.error_x, 0.0F);
  EXPECT_FLOAT_EQ(command.error_x, 100.0F);
  EXPECT_FLOAT_EQ(command.target_x, 420.0F);
}

TEST(TurretDecisionTest, TargetOnCenterXProducesServoCenter)
{
  const auto command = antidrone_turret::compute_servo_command(
    TargetObservation{true, 320.0F, 240.0F, 25.0F, 0.90F});

  EXPECT_EQ(command.direction, 0);
  EXPECT_FLOAT_EQ(command.error_x, 0.0F);
}

TEST(TurretDecisionTest, TargetAboveCenterProducesGimbalUp)
{
  const auto command = antidrone_turret::compute_gimbal_command(
    TargetObservation{true, 320.0F, 180.0F, 25.0F, 0.90F});

  EXPECT_EQ(command.direction, 1);
  EXPECT_GT(command.error_y, 0.0F);
  EXPECT_FLOAT_EQ(command.error_y, 60.0F);
  EXPECT_FLOAT_EQ(command.target_y, 180.0F);
}

TEST(TurretDecisionTest, TargetBelowCenterProducesGimbalDown)
{
  const auto command = antidrone_turret::compute_gimbal_command(
    TargetObservation{true, 320.0F, 300.0F, 25.0F, 0.90F});

  EXPECT_EQ(command.direction, -1);
  EXPECT_LT(command.error_y, 0.0F);
}

TEST(TurretDecisionTest, CloseTargetWithReadyActuatorRequestsTrigger)
{
  const auto target = TargetObservation{true, 320.0F, 240.0F, 25.0F, 0.90F};

  const auto decision = antidrone_turret::decide(target, true, default_config());

  EXPECT_EQ(decision.status.trigger_state, TriggerDecision::kRequested);
  EXPECT_TRUE(decision.should_request_trigger);
}

TEST(TurretDecisionTest, CloseTargetWithReloadingActuatorSkipsRequest)
{
  const auto target = TargetObservation{true, 320.0F, 240.0F, 25.0F, 0.90F};

  const auto decision = antidrone_turret::decide(target, false, default_config());

  EXPECT_EQ(decision.status.trigger_state, TriggerDecision::kReloading);
  EXPECT_FALSE(decision.should_request_trigger);
}

TEST(TurretDecisionTest, FarTargetSkipsTriggerRegardlessOfActuator)
{
  const auto target = TargetObservation{true, 320.0F, 240.0F, 45.0F, 0.90F};

  const auto decision = antidrone_turret::decide(target, true, default_config());

  EXPECT_EQ(decision.status.trigger_state, TriggerDecision::kSkip);
  EXPECT_FALSE(decision.should_request_trigger);
}

TEST(TurretDecisionTest, FarLockedTargetStillTracksButSkipsTrigger)
{
  const auto target = TargetObservation{true, 445.0F, 170.0F, 70.0F, 0.95F};

  const auto decision = antidrone_turret::decide(target, true, default_config());

  EXPECT_EQ(decision.status.target_state, TargetState::kLocked);
  EXPECT_EQ(decision.status.action, TurretAction::kTrack);
  EXPECT_EQ(decision.status.trigger_state, TriggerDecision::kSkip);
  EXPECT_TRUE(decision.should_publish_aim_commands);
  EXPECT_FALSE(decision.should_request_trigger);
}

}  // namespace
