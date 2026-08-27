#include <cstdint>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include "antidrone_turret/msg/servo_command.hpp"

namespace {

constexpr auto kServoCommandTopic = "/servo/cmd";

const char* direction_name(const std::int8_t direction)
{
  using ServoCommand = antidrone_turret::msg::ServoCommand;
  if (direction == ServoCommand::RIGHT) {
    return "RIGHT";
  }
  if (direction == ServoCommand::LEFT) {
    return "LEFT";
  }
  return "CENTER";
}

}  // namespace

class YawServoDriverNode final : public rclcpp::Node {
public:
  using ServoCommand = antidrone_turret::msg::ServoCommand;

  YawServoDriverNode()
    : Node("yaw_servo_driver_node")
  {
    subscription_ = create_subscription<ServoCommand>(
      kServoCommandTopic,
      10,
      [this](const ServoCommand& command) { on_command(command); });

    RCLCPP_INFO(get_logger(), "listening on %s", kServoCommandTopic);
  }

private:
  void on_command(const ServoCommand& command)
  {
    RCLCPP_INFO(
      get_logger(),
      "yaw_servo_driver_node отримав: direction=%s target_x=%.1f error_x=%.1f",
      direction_name(command.direction),
      static_cast<double>(command.target_x),
      static_cast<double>(command.error_x));
  }

  rclcpp::Subscription<ServoCommand>::SharedPtr subscription_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<YawServoDriverNode>());
  rclcpp::shutdown();
  return 0;
}
