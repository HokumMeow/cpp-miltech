#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"

#include "underground_world/msg/enemy_down.hpp"
#include "underground_world/srv/payload_trigger.hpp"

namespace {

using underground_world::msg::EnemyDown;
using underground_world::srv::PayloadTrigger;

constexpr auto kEnemyDownTopic = "/payload/enemy_down";
constexpr auto kTriggerService = "/payload/trigger";

}

class PayloadActionNode final : public rclcpp::Node {
public:
  PayloadActionNode()
    : Node("payload_action_node")
  {
    enemy_down_pub_ = create_publisher<EnemyDown>(kEnemyDownTopic, rclcpp::QoS{10});

    trigger_service_ = create_service<PayloadTrigger>(
      kTriggerService,
      [this](const std::shared_ptr<PayloadTrigger::Request> request,
             std::shared_ptr<PayloadTrigger::Response> response) { handle_trigger(request, response); });

    RCLCPP_INFO(get_logger(), "payload_action_node ready, serving %s", kTriggerService);
  }

private:
  void handle_trigger(const std::shared_ptr<PayloadTrigger::Request>& request,
                       const std::shared_ptr<PayloadTrigger::Response>& response)
  {
    EnemyDown enemy_down;
    enemy_down.contact_id = request->contact_id;
    enemy_down.x = request->x;
    enemy_down.y = request->y;
    enemy_down_pub_->publish(enemy_down);

    response->accepted = true;
    response->reason = "payload triggered for contact_id=" + std::to_string(request->contact_id);

    RCLCPP_INFO(get_logger(),
                "trigger contact_id=%d position=(%d,%d) -> enemy_down published",
                request->contact_id,
                request->x,
                request->y);
  }

  rclcpp::Publisher<EnemyDown>::SharedPtr enemy_down_pub_;
  rclcpp::Service<PayloadTrigger>::SharedPtr trigger_service_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<PayloadActionNode>());
  rclcpp::shutdown();
  return 0;
}
