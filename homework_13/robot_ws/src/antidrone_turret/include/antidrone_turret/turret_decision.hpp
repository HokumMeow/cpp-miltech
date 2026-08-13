#pragma once

#include <cstdint>

namespace antidrone_turret {

enum class TargetState : std::uint8_t {
  kNone = 0,
  kLowConfidence = 1,
  kLocked = 2,
};

enum class TurretAction : std::uint8_t {
  kIdle = 0,
  kTrack = 1,
};

enum class TriggerDecision : std::uint8_t {
  kSkip = 0,
  kRequested = 1,
  kReloading = 2,
};

struct TurretDecisionConfig {
  float confidence_threshold{0.80F};
  float max_distance_m{30.0F};
};

struct TargetObservation {
  bool visible{false};
  float x{0.0F};
  float y{0.0F};
  float distance_m{0.0F};
  float confidence{0.0F};
};

struct GimbalCommandDto {
  std::int8_t direction{0};
  float target_y{0.0F};
  float error_y{0.0F};
};

struct ServoCommandDto {
  std::int8_t direction{0};
  float target_x{0.0F};
  float error_x{0.0F};
};

struct TurretStatusDto {
  TargetState target_state{TargetState::kNone};
  TurretAction action{TurretAction::kIdle};
  TriggerDecision trigger_state{TriggerDecision::kSkip};
  float confidence{0.0F};
  float distance_m{0.0F};
};

struct TurretDecision {
  TurretStatusDto status;
  GimbalCommandDto gimbal;
  ServoCommandDto servo;
  bool should_publish_aim_commands{false};
  bool should_request_trigger{false};
};

[[nodiscard]] inline TargetState evaluate_target_state(
  const TargetObservation& target,
  const TurretDecisionConfig& config)
{
  if (!target.visible) {
    return TargetState::kNone;
  }
  if (target.confidence < config.confidence_threshold) {
    return TargetState::kLowConfidence;
  }
  return TargetState::kLocked;
}

[[nodiscard]] inline ServoCommandDto compute_servo_command(const TargetObservation& target)
{
  ServoCommandDto command;
  command.target_x = target.x;
  command.error_x = target.x - 320.0F;

  if (command.error_x > 0.0F) {
    command.direction = 1;
  } else if (command.error_x < 0.0F) {
    command.direction = -1;
  } else {
    command.direction = 0;
  }

  return command;
}

[[nodiscard]] inline GimbalCommandDto compute_gimbal_command(const TargetObservation& target)
{
  GimbalCommandDto command;
  command.target_y = target.y;
  command.error_y = 240.0F - target.y;

  if (command.error_y > 0.0F) {
    command.direction = 1;   // UP
  } else if (command.error_y < 0.0F) {
    command.direction = -1;  // DOWN
  } else {
    command.direction = 0;   // CENTER
  }

  return command;
}

[[nodiscard]] inline TriggerDecision decide_trigger(
  const float distance_m,
  const bool actuator_ready,
  const TurretDecisionConfig& config)
{
  if (distance_m > config.max_distance_m) {
    return TriggerDecision::kSkip;
  }
  return actuator_ready ? TriggerDecision::kRequested : TriggerDecision::kReloading;
}

[[nodiscard]] inline TurretDecision decide(
  const TargetObservation& target,
  const bool actuator_ready,
  const TurretDecisionConfig& config)
{
  TurretDecision decision;
  decision.status.confidence = target.confidence;
  decision.status.distance_m = target.distance_m;

  const auto target_state = evaluate_target_state(target, config);
  decision.status.target_state = target_state;

  if (target_state != TargetState::kLocked) {
    decision.status.action = TurretAction::kIdle;
    decision.status.trigger_state = TriggerDecision::kSkip;
    return decision;
  }

  decision.status.action = TurretAction::kTrack;
  decision.should_publish_aim_commands = true;
  decision.gimbal = compute_gimbal_command(target);
  decision.servo = compute_servo_command(target);

  const auto trigger_decision = decide_trigger(target.distance_m, actuator_ready, config);
  decision.status.trigger_state = trigger_decision;
  decision.should_request_trigger = trigger_decision == TriggerDecision::kRequested;

  return decision;
}

} 
