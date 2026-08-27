#include "c2_controller.hpp"
#include "fc_link.hpp"     // MAVSDK обгортка, API описано у fc_link.hpp
#include "udp_socket.hpp"  // UDP прийом, API описано у udp_socket.hpp

#include <nlohmann/json.hpp>  // Розбiр JSON з точками маршруту вiд auto_stub

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

static constexpr uint16_t STUB_PORT = 14560;

namespace {

const char* state_name(C2State s) {
    switch (s) {
        case C2State::DISARMED:     return "DISARMED";
        case C2State::ARMED_HOLD:   return "ARMED_HOLD";
        case C2State::ARMED_GUIDED: return "ARMED_GUIDED";
        case C2State::ARMED_MANUAL: return "ARMED_MANUAL";
    }
    return "UNKNOWN";
}


C2State target_state(bool armed, FcLink::FlightMode mode) {
    if (!armed) return C2State::DISARMED;

    switch (mode) {
        case FcLink::FlightMode::Guided: return C2State::ARMED_GUIDED;
        case FcLink::FlightMode::Hold:   return C2State::ARMED_HOLD;
        case FcLink::FlightMode::Manual:
        default:                         return C2State::ARMED_MANUAL;
    }
}

}  // namespace

struct C2Controller::Impl {
    explicit Impl(uint16_t fc_port)
        : fc(fc_port), udp(STUB_PORT)
    {
        log_file.open("/var/log/c2/c2.log", std::ios::app);
    }

    C2State state = C2State::DISARMED;

    FcLink fc;
    UdpSocket udp;
    std::ofstream log_file;

    bool healthy_marked = false;

    void log(const std::string& line) {
        std::cout << line << std::endl;
        if (log_file.is_open()) {
            log_file << line << std::endl;
            log_file.flush();
        }
    }

    // checking state changing
    bool transition(C2State next) {
        if (next == state) return false;
        log(std::string("[C2] state: ") + state_name(state) + " -> " + state_name(next));
        state = next;
        return true;
    }
};

C2Controller::C2Controller(uint16_t fc_port)
    : impl_(std::make_unique<Impl>(fc_port))
{
}

C2Controller::~C2Controller() = default;

void C2Controller::tick() {
    Impl& s = *impl_;

    // Self-test / healthcheck - ready after first heartbeat
    if (!s.healthy_marked && s.fc.is_connected()) {
        std::ofstream("/tmp/c2_healthy").close();
        s.healthy_marked = true;
    }

    // update C2 state
    const C2State next = target_state(s.fc.is_armed(), s.fc.flight_mode());
    const bool changed = s.transition(next);

    if (changed && s.state == C2State::ARMED_HOLD) {
        s.fc.hold();
    }

    // read waypoints from auto_stub in this tick
    char buf[512];
    for (;;) {
        const ssize_t n = s.udp.recv(buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';

        nlohmann::json wp;
        try {
            wp = nlohmann::json::parse(buf, buf + n);
        } catch (const nlohmann::json::exception& e) {
            s.log(std::string("[C2] error: bad waypoint json: ") + e.what());
            continue;
        }

        if (s.state == C2State::ARMED_GUIDED) {
            const float north = wp.at("north_m").get<float>();
            const float east  = wp.at("east_m").get<float>();
            s.fc.go_to_ned(north, east);

            std::ostringstream oss;
            oss << "[C2] fwd: north=" << north << " east=" << east;
            s.log(oss.str());
        } else {
            s.log(std::string("[C2] blocked: waypoint in ") + state_name(s.state));
        }
    }
}

C2State C2Controller::current_state() const {
    return impl_->state;
}
