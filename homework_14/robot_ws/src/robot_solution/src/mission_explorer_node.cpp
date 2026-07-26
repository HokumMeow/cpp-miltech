#include <algorithm>
#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <tuple>
#include <vector>

#include "rclcpp/rclcpp.hpp"

#include "underground_world/msg/cell_observation.hpp"
#include "underground_world/msg/local_scan.hpp"
#include "underground_world/msg/move_command.hpp"
#include "underground_world/msg/robot_result.hpp"
#include "underground_world/msg/student_status.hpp"
#include "underground_world/srv/payload_trigger.hpp"
#include "underground_world/state_qos.hpp"

namespace {

using underground_world::msg::CellObservation;
using underground_world::msg::LocalScan;
using underground_world::msg::MoveCommand;
using underground_world::msg::RobotResult;
using underground_world::msg::StudentStatus;
using underground_world::srv::PayloadTrigger;

constexpr auto kScanTopic = "/robot/local_scan";
constexpr auto kResultTopic = "/robot/result";
constexpr auto kMoveTopic = "/robot/cmd_move";
constexpr auto kStatusTopic = "/student/status";
constexpr auto kTriggerService = "/payload/trigger";

struct Position {
  std::int32_t x = 0;
  std::int32_t y = 0;
};

bool operator==(const Position& lhs, const Position& rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}

bool operator<(const Position& lhs, const Position& rhs)
{
  return std::tie(lhs.x, lhs.y) < std::tie(rhs.x, rhs.y);
}

constexpr int kStepDx[4] = {0, 0, -1, 1};
constexpr int kStepDy[4] = {-1, 1, 0, 0};

bool is_steppable(const char cell_type)
{
  return cell_type == '.' || cell_type == 'S' || cell_type == 'x';
}

}

class MissionExplorerNode final : public rclcpp::Node {
public:
  MissionExplorerNode()
    : Node("mission_explorer_node")
  {
    const auto event_qos = rclcpp::QoS{10};
    const auto state_qos = underground_world::make_state_qos();

    move_pub_ = create_publisher<MoveCommand>(kMoveTopic, event_qos);
    status_pub_ = create_publisher<StudentStatus>(kStatusTopic, event_qos);

    scan_sub_ = create_subscription<LocalScan>(
      kScanTopic, state_qos, [this](const LocalScan::SharedPtr msg) { on_scan(*msg); });

    result_sub_ = create_subscription<RobotResult>(
      kResultTopic, state_qos, [this](const RobotResult::SharedPtr msg) { on_result(*msg); });

    trigger_client_ = create_client<PayloadTrigger>(kTriggerService);

    retry_timer_ = create_wall_timer(std::chrono::milliseconds{100}, [this]() {
      if (!has_pending_retry_) {
        return;
      }
      has_pending_retry_ = false;
      evaluate_and_act();
    });

    RCLCPP_INFO(get_logger(), "mission_explorer_node ready");
  }

private:

  void on_scan(const LocalScan& scan)
  {
    latest_scan_ = scan;
    have_scan_ = true;

    const Position robot{scan.robot_x, scan.robot_y};
    if (!start_position_.has_value()) {
      start_position_ = robot;
    }

    for (const auto& cell : scan.cells) {
      const Position position{cell.x, cell.y};
      const char kind = cell.cell_type.empty() ? '#' : cell.cell_type.front();
      known_cells_[position] = kind;

      if (kind == 'x') {
        processed_contacts_.insert(cell.contact_id);
        pending_triggers_.erase(cell.contact_id);
      }
    }

    ++scan_update_count_;
    request_evaluation();
  }

  void on_result(const RobotResult& result)
  {
    have_result_ = true;
    mission_result_ = result.mission_result;
    ++result_update_count_;
    request_evaluation();
  }

  void request_evaluation()
  {
    if (scan_update_count_ != result_update_count_ || scan_update_count_ == last_evaluated_count_) {
      return;
    }
    last_evaluated_count_ = scan_update_count_;
    evaluate_and_act();
  }

  void evaluate_and_act()
  {
    if (finished_ || !have_scan_ || !have_result_) {
      return;
    }

    if (mission_result_ == "FAILED_MAX_STEPS") {
      finish_with(StudentStatus::FAILED, "max_steps budget exhausted");
      return;
    }

    const auto visible_unprocessed = visible_unprocessed_contacts();
    if (!visible_unprocessed.empty()) {
      set_state(StudentStatus::ENGAGING);
      for (const auto& cell : visible_unprocessed) {
        send_trigger(cell);
      }
      return;
    }

    if (!pending_triggers_.empty()) {
      set_state(StudentStatus::ENGAGING);
      return;
    }

    if (mission_result_ == "SUCCESS") {
      const Position robot = robot_position();
      if (start_position_.has_value() && robot == *start_position_) {
        finish_with(StudentStatus::DONE, "mission complete, returned to start");
      }
      else {
        set_state(StudentStatus::RETURNING);
        if (!move_towards(*start_position_)) {
          finish_with(StudentStatus::FAILED, "cannot find a known path back to start");
        }
      }
      return;
    }

    set_state(StudentStatus::EXPLORING);
    const auto frontier = find_nearest_frontier();
    if (frontier.has_value()) {
      stuck_ticks_ = 0;
      move_towards(*frontier);
      return;
    }

    if (++stuck_ticks_ > kStuckLimit) {
      finish_with(StudentStatus::FAILED, "no reachable frontier and mission still running");
    }
  }

  std::vector<CellObservation> visible_unprocessed_contacts() const
  {
    std::vector<CellObservation> result;
    for (const auto& cell : latest_scan_.cells) {
      if (cell.cell_type != "C") {
        continue;
      }
      if (processed_contacts_.count(cell.contact_id) != 0 || pending_triggers_.count(cell.contact_id) != 0) {
        continue;
      }
      result.push_back(cell);
    }
    return result;
  }

  void send_trigger(const CellObservation& cell)
  {
    if (!trigger_client_->service_is_ready()) {
      has_pending_retry_ = true;
      RCLCPP_INFO(get_logger(), "trigger service not ready yet, will retry contact_id=%d", cell.contact_id);
      return;
    }

    pending_triggers_.insert(cell.contact_id);

    auto request = std::make_shared<PayloadTrigger::Request>();
    request->contact_id = cell.contact_id;
    request->x = cell.x;
    request->y = cell.y;

    trigger_client_->async_send_request(
      request, [this, contact_id = cell.contact_id](rclcpp::Client<PayloadTrigger>::SharedFuture future) {
        const auto response = future.get();
        RCLCPP_INFO(get_logger(),
                    "trigger response contact_id=%d accepted=%s reason=%s",
                    contact_id,
                    response->accepted ? "true" : "false",
                    response->reason.c_str());
      });

    RCLCPP_INFO(get_logger(), "trigger requested contact_id=%d position=(%d,%d)", cell.contact_id, cell.x, cell.y);
  }

  Position robot_position() const
  {
    return Position{latest_scan_.robot_x, latest_scan_.robot_y};
  }

  bool is_frontier(const Position position) const
  {
    for (int dy = -1; dy <= 1; ++dy) {
      for (int dx = -1; dx <= 1; ++dx) {
        if (dx == 0 && dy == 0) {
          continue;
        }
        const Position neighbor{position.x + dx, position.y + dy};
        if (known_cells_.find(neighbor) == known_cells_.end()) {
          return true;
        }
      }
    }
    return false;
  }

  std::optional<std::vector<Position>> bfs_to(const Position start, const std::function<bool(Position)>& is_goal) const
  {
    std::queue<Position> pending;
    std::map<Position, Position> parent;
    std::set<Position> visited;

    pending.push(start);
    visited.insert(start);

    while (!pending.empty()) {
      const Position current = pending.front();
      pending.pop();

      if (is_goal(current)) {
        std::vector<Position> path{current};
        while (!(path.back() == start)) {
          path.push_back(parent.at(path.back()));
        }
        std::reverse(path.begin(), path.end());
        return path;
      }

      for (int i = 0; i < 4; ++i) {
        const Position next{current.x + kStepDx[i], current.y + kStepDy[i]};
        if (visited.count(next) != 0) {
          continue;
        }
        const auto it = known_cells_.find(next);
        if (it == known_cells_.end() || !is_steppable(it->second)) {
          continue;
        }
        visited.insert(next);
        parent[next] = current;
        pending.push(next);
      }
    }

    return std::nullopt;
  }

  std::optional<Position> find_nearest_frontier() const
  {
    const Position start = robot_position();
    const auto path = bfs_to(start, [this, start](const Position candidate) {
      return !(candidate == start) && is_frontier(candidate);
    });
    if (!path.has_value()) {
      return std::nullopt;
    }
    return path->back();
  }

  bool move_towards(const Position goal)
  {
    const Position start = robot_position();
    const auto path = bfs_to(start, [goal](const Position candidate) { return candidate == goal; });
    if (!path.has_value() || path->size() < 2) {
      return false;
    }

    if (move_pub_->get_subscription_count() == 0) {
      has_pending_retry_ = true;
      RCLCPP_INFO(get_logger(), "cmd_move has no matched subscriber yet, deferring move");
      return false;
    }

    publish_move(direction_between(path->at(0), path->at(1)));
    return true;
  }

  static std::uint8_t direction_between(const Position from, const Position to)
  {
    if (to.x == from.x && to.y == from.y - 1) {
      return MoveCommand::UP;
    }
    if (to.x == from.x && to.y == from.y + 1) {
      return MoveCommand::DOWN;
    }
    if (to.x == from.x - 1 && to.y == from.y) {
      return MoveCommand::LEFT;
    }
    return MoveCommand::RIGHT;
  }

  void publish_move(const std::uint8_t direction)
  {
    MoveCommand msg;
    msg.direction = direction;
    move_pub_->publish(msg);
  }

  void set_state(const std::uint8_t state)
  {
    current_state_ = state;
    publish_status();
  }

  void finish_with(const std::uint8_t state, const std::string& reason)
  {
    current_state_ = state;
    finished_ = true;
    publish_status();
    RCLCPP_INFO(get_logger(), "mission_explorer finished state=%u reason=%s", static_cast<unsigned>(state), reason.c_str());
  }

  void publish_status()
  {
    StudentStatus msg;
    msg.state = current_state_;
    status_pub_->publish(msg);
  }

  static constexpr int kStuckLimit = 3;

  LocalScan latest_scan_;
  bool have_scan_ = false;
  std::string mission_result_ = "RUNNING";
  bool have_result_ = false;

  std::optional<Position> start_position_;
  std::map<Position, char> known_cells_;
  std::set<std::int32_t> processed_contacts_;
  std::set<std::int32_t> pending_triggers_;

  std::uint8_t current_state_ = StudentStatus::EXPLORING;
  bool finished_ = false;
  int stuck_ticks_ = 0;
  bool has_pending_retry_ = false;
  std::uint64_t scan_update_count_ = 0;
  std::uint64_t result_update_count_ = 0;
  std::uint64_t last_evaluated_count_ = 0;

  rclcpp::Publisher<MoveCommand>::SharedPtr move_pub_;
  rclcpp::Publisher<StudentStatus>::SharedPtr status_pub_;
  rclcpp::Subscription<LocalScan>::SharedPtr scan_sub_;
  rclcpp::Subscription<RobotResult>::SharedPtr result_sub_;
  rclcpp::Client<PayloadTrigger>::SharedPtr trigger_client_;
  rclcpp::TimerBase::SharedPtr retry_timer_;
};

int main(int argc, char** argv)
{
  rclcpp::init(argc, argv);
  rclcpp::spin(std::make_shared<MissionExplorerNode>());
  rclcpp::shutdown();
  return 0;
}
