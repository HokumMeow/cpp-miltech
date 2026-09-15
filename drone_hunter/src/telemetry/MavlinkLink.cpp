#include "telemetry/MavlinkLink.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cmath>
#include <cstring>
#include <thread>
#include <utility>

#include "dto/DroneTelemetry.h"
#include "interfaces/IDronePhysics.h"
#include "Log.h"

namespace {

// GPS reference point
constexpr double kLat0 = 50.4501;
constexpr double kLon0 = 30.5234;
constexpr double kMetersPerDegLat = 111320.0;

constexpr float kPi = 3.14159265f;

constexpr auto kHeartbeatPeriod = std::chrono::milliseconds(1000);
constexpr auto kTelemetryPeriod = std::chrono::milliseconds(200);
constexpr auto kDropRetryTimeout = std::chrono::milliseconds(500);
constexpr auto kLoopTick = std::chrono::milliseconds(20);

std::pair<double, double> localToGps(float x, float y) {
    const double lat = kLat0 + (static_cast<double>(y) / kMetersPerDegLat);
    const double lon = kLon0 + (static_cast<double>(x) / (kMetersPerDegLat * std::cos(kLat0 * M_PI / 180.0)));
    return {lat, lon};
}

}  // namespace

MavlinkLink::MavlinkLink(IDronePhysics& physics, const std::string& host, uint16_t port, float altitude)
    : physics_(physics), altitude_(altitude) {
    sock_ = socket(AF_INET, SOCK_DGRAM, 0);
    const int flags = fcntl(sock_, F_GETFL, 0);
    fcntl(sock_, F_SETFL, flags | O_NONBLOCK);

    destAddr_.sin_family = AF_INET;
    destAddr_.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &destAddr_.sin_addr);
}

MavlinkLink::~MavlinkLink() {
    if (sock_ >= 0) {
        close(sock_);
    }
}

void MavlinkLink::sendBuffer(const uint8_t* data, int len) {
    sendto(sock_, data, static_cast<size_t>(len), 0,
           reinterpret_cast<const sockaddr*>(&destAddr_), sizeof destAddr_);
}

void MavlinkLink::sendHeartbeat() {
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    mavlink_msg_heartbeat_pack(kSysId, MAV_COMP_ID_AUTOPILOT1, &msg,
                                MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_GENERIC,
                                0, 0, MAV_STATE_ACTIVE);
    const int len = mavlink_msg_to_send_buffer(buf, &msg);
    sendBuffer(buf, len);
}

void MavlinkLink::sendTelemetry() {
    const DroneTelemetry tel = physics_.getTelemetry();
    const auto [lat, lon] = localToGps(tel.pos.x, tel.pos.y);

    const auto timeBootMs = static_cast<uint32_t>(tel.timeSecSinceStart * 1000.f);
    const auto latE7 = static_cast<int32_t>(lat * 1e7);
    const auto lonE7 = static_cast<int32_t>(lon * 1e7);
    const auto altMm = static_cast<int32_t>(altitude_ * 1000.f);
    const auto vx = static_cast<int16_t>(tel.speed.x * 100.f);
    const auto vy = static_cast<int16_t>(tel.speed.y * 100.f);

    float hdgDeg = tel.direction * 180.f / kPi;
    while (hdgDeg < 0.f) hdgDeg += 360.f;
    while (hdgDeg >= 360.f) hdgDeg -= 360.f;
    const auto hdg = static_cast<uint16_t>(hdgDeg * 100.f);

    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];

    mavlink_msg_global_position_int_pack(kSysId, MAV_COMP_ID_AUTOPILOT1, &msg,
                                          timeBootMs, latE7, lonE7, altMm, altMm,
                                          vx, vy, 0, hdg);
    int len = mavlink_msg_to_send_buffer(buf, &msg);
    sendBuffer(buf, len);

    mavlink_msg_attitude_pack(kSysId, MAV_COMP_ID_AUTOPILOT1, &msg,
                               timeBootMs, 0.f, 0.f, tel.direction, 0.f, 0.f, 0.f);
    len = mavlink_msg_to_send_buffer(buf, &msg);
    sendBuffer(buf, len);
}

void MavlinkLink::pollIncoming() {
    uint8_t buf[512];
    while (true) {
        const ssize_t n = recv(sock_, buf, sizeof buf, 0);
        if (n <= 0) {
            if (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                LOG("MAVLink: recv error");
            }
            return;
        }

        for (ssize_t i = 0; i < n; ++i) {
            mavlink_message_t msg;
            if (!mavlink_parse_char(MAVLINK_COMM_0, buf[static_cast<size_t>(i)], &msg, &rxStatus_)) {
                continue;
            }
            if (msg.msgid != MAVLINK_MSG_ID_COMMAND_ACK) {
                continue;
            }

            mavlink_command_ack_t ack;
            mavlink_msg_command_ack_decode(&msg, &ack);

            std::lock_guard<std::mutex> lk(dropMtx_);
            if (dropPending_ && ack.command == MAV_CMD_USER_1) {
                if (ack.result == MAV_RESULT_ACCEPTED) {
                    LOG("MAVLink: drop command ACKed");
                } else {
                    LOG("MAVLink: drop command ACK with result=" << static_cast<int>(ack.result));
                }
                dropPending_ = false;
            }
        }
    }
}

void MavlinkLink::serviceDropRetries() {
    std::lock_guard<std::mutex> lk(dropMtx_);
    if (!dropPending_) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (dropAttempts_ > 0 && now - lastDropSend_ < kDropRetryTimeout) {
        return;
    }

    if (dropAttempts_ >= kMaxDropAttempts) {
        LOG("MAVLink: drop command ACK not received after " << kMaxDropAttempts << " attempts");
        dropPending_ = false;
        return;
    }

    const auto [lat, lon] = localToGps(dropPoint_.x, dropPoint_.y);

    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    mavlink_msg_command_long_pack(kSysId, MAV_COMP_ID_AUTOPILOT1, &msg,
                                   0, 0, MAV_CMD_USER_1, static_cast<uint8_t>(dropAttempts_),
                                   0.f, 0.f, 0.f, 0.f,
                                   static_cast<float>(lat), static_cast<float>(lon), dropAlt_);
    const int len = mavlink_msg_to_send_buffer(buf, &msg);
    sendBuffer(buf, len);

    ++dropAttempts_;
    lastDropSend_ = now;
}

void MavlinkLink::reportDrop(Coord dropPoint, float altitude) {
    std::lock_guard<std::mutex> lk(dropMtx_);
    dropPending_ = true;
    dropAttempts_ = 0;
    dropPoint_ = dropPoint;
    dropAlt_ = altitude;
}

bool MavlinkLink::hasPendingDrop() const {
    std::lock_guard<std::mutex> lk(dropMtx_);
    return dropPending_;
}

void MavlinkLink::run() {
    ready_.store(true);
    while (!started_.load() && !stopFlag_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    auto lastHb = std::chrono::steady_clock::now() - kHeartbeatPeriod;
    auto lastTel = std::chrono::steady_clock::now() - kTelemetryPeriod;

    while (!stopFlag_.load()) {
        const auto now = std::chrono::steady_clock::now();

        if (now - lastHb >= kHeartbeatPeriod) {
            sendHeartbeat();
            lastHb = now;
        }
        if (now - lastTel >= kTelemetryPeriod) {
            sendTelemetry();
            lastTel = now;
        }

        pollIncoming();
        serviceDropRetries();

        std::this_thread::sleep_for(kLoopTick);
    }
}
