#include <cstdint>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "antidrone_turret/turret_decision.hpp"
#include "antidrone_turret/msg/actuator_status.hpp"
#include "antidrone_turret/msg/gimbal_command.hpp"
#include "antidrone_turret/msg/servo_command.hpp"
#include "antidrone_turret/msg/target.hpp"
#include "antidrone_turret/msg/turret_status.hpp"
#include "antidrone_turret/srv/trigger_actuator.hpp"

namespace {

constexpr auto kTargetTopic = "/perception/target";
constexpr auto kActuatorStatusTopic = "/actuator/status";
constexpr auto kGimbalCommandTopic = "/gimbal/cmd";
constexpr auto kServoCommandTopic = "/servo/cmd";
constexpr auto kTurretStatusTopic = "/turret/status";
constexpr auto kTriggerService = "/actuator/trigger";

antidrone_turret::msg::GimbalCommand to_message(const antidrone_turret::GimbalCommandDto& dto)
{
  auto message = antidrone_turret::msg::GimbalCommand{};
  message.direction = dto.direction;
  message.target_y = dto.target_y;
  message.error_y = dto.error_y;
  return message;
}

antidrone_turret::msg::ServoCommand to_message(const antidrone_turret::ServoCommandDto& dto)
{
  auto message = antidrone_turret::msg::ServoCommand{};
  message.direction = dto.direction;
  message.target_x = dto.target_x;
  message.error_x = dto.error_x;
  return message;
}

antidrone_turret::msg::TurretStatus to_message(const antidrone_turret::TurretStatusDto& dto)
{
  auto message = antidrone_turret::msg::TurretStatus{};
  message.target_state = static_cast<std::uint8_t>(dto.target_state);
  message.action = static_cast<std::uint8_t>(dto.action);
  message.trigger_state = static_cast<std::uint8_t>(dto.trigger_state);
  message.confidence = dto.confidence;
  message.distance_m = dto.distance_m;
  return message;
}

}  // namespace

class TurretControllerNode final : public rclcpp::Node {
public:
  using Target = antidrone_turret::msg::Target;
  using ActuatorStatus = antidrone_turret::msg::ActuatorStatus;
  using GimbalCommand = antidrone_turret::msg::GimbalCommand;
  using ServoCommand = antidrone_turret::msg::ServoCommand;
  using TurretStatus = antidrone_turret::msg::TurretStatus;
  using TriggerActuator = antidrone_turret::srv::TriggerActuator;

  TurretControllerNode()
    : Node("turret_controller_node")
  {
    config_.confidence_threshold =
      static_cast<float>(declare_parameter<double>("confidence_threshold", 0.80));
    config_.max_distance_m =
      static_cast<float>(declare_parameter<double>("max_distance_m", 30.0));

    gimbal_publisher_ = create_publisher<GimbalCommand>(kGimbalCommandTopic, 10);
    servo_publisher_ = create_publisher<ServoCommand>(kServoCommandTopic, 10);
    status_publisher_ = create_publisher<TurretStatus>(kTurretStatusTopic, 10);
    trigger_client_ = create_client<TriggerActuator>(kTriggerService);

    actuator_status_subscription_ = create_subscription<ActuatorStatus>(
      kActuatorStatusTopic,
      10,
      [this](const ActuatorStatus& status) { on_actuator_status(status); });

    target_subscription_ = create_subscription<Target>(
      kTargetTopic,
      10,
      [this](const Target& target) { on_target(target); });

    RCLCPP_INFO(
      get_logger(),
      "turret_controller_node ready: confidence_threshold=%.2f max_distance_m=%.1f",
      static_cast<double>(config_.confidence_threshold),
      static_cast<double>(config_.max_distance_m));
  }

private:
  void on_actuator_status(const ActuatorStatus& status)
  {
    actuator_ready_ = status.state == ActuatorStatus::READY;
  }

  void on_target(const Target& target)
  {
    const auto observation = antidrone_turret::TargetObservation{
      target.visible,
      target.x,
      target.y,
      target.distance_m,
      target.confidence,
    };

    const auto decision = antidrone_turret::decide(observation, actuator_ready_, config_);

    status_publisher_->publish(to_message(decision.status));

    if (decision.should_publish_aim_commands) {
      gimbal_publisher_->publish(to_message(decision.gimbal));
      servo_publisher_->publish(to_message(decision.servo));
    }

    if (decision.should_request_trigger) {
      request_trigger(target.confidence, target.distance_m);
    }
  }

  void request_trigger(const float confidence, const float distance_m)
  {
    auto request = std::make_shared<TriggerActuator::Request>();
    request->confidence = confidence;
    request->distance_m = distance_m;

    trigger_client_->async_send_request(
      request,
      [this](const rclcpp::Client<TriggerActuator>::SharedFuture future) {
        const auto response = future.get();
        RCLCPP_INFO(
          get_logger(),
          "trigger request accepted=%s trigger_count=%u",
          response->accepted ? "true" : "false",
          response->trigger_count);
      });
  }

  antidrone_turret::TurretDecisionConfig config_{};
  bool actuator_ready_{true};

  rclcpp::Publisher<GimbalCommand>::SharedPtr gimbal_publisher_;
  rclcpp::Publisher<ServoCommand>::SharedPtr servo_publisher_;
  rclcpp::Publisher<TurretStatus>::SharedPtr status_publisher_;
  rclcpp::Client<TriggerActuator>::SharedPtr trigger_client_;
  rclcpp::Subscription<ActuatorStatus>::SharedPtr actuator_status_subscription_;
  rclcpp::Subscription<Target>::SharedPtr target_subscription_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<TurretControllerNode>());
  rclcpp::shutdown();
  return 0;
}
