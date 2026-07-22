#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <link/UartLink.h>
#include <thread>

UartLink::UartLink(const std::string& port) {
    fd_ = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        std::cerr << "Failed to open serial port" << std::endl;
        return;
    }

    termios tio;
    tcgetattr(fd_, &tio);
    cfmakeraw(&tio);
    cfsetispeed(&tio, B115200);
    cfsetospeed(&tio, B115200);
    tio.c_cflag |= (CLOCAL | CREAD);
    tcsetattr(fd_, TCSANOW, &tio);
}

void UartLink::sendControl(float accel, float turnRate) {
    dlink::Control c{ accel, turnRate };
    uint8_t out[64];
    size_t m = dlink::encode(dlink::PKT_CONTROL, &c, sizeof c, out);
    write(fd_, out, m);
}

void UartLink::run() {
    ready_.store(true);
    while (!started_.load() && !stopFlag_.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    while (!stopFlag_.load()) {
        uint8_t buf[256];
        int n = read(fd_, buf, sizeof(buf));

        if (n < 0) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                std::cerr << "Error reading from serial port" << std::endl;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        } else {
            for (int i = 0; i < n; i++) {
                uint8_t type, len, payload[260];
                if (parser_.feed(buf[i], type, payload, len)) {
                    std::lock_guard<std::mutex> lk(mtx_);
                    switch (type) {
                        case dlink::PKT_TELEMETRY: memcpy(&telemetry_, payload, sizeof telemetry_); hasTelemetry_.store(true); break;
                        case dlink::PKT_TARGET: 
                            if (len == sizeof(dlink::TargetPos)) {
                                dlink::TargetPos target;
                                memcpy(&target, payload, sizeof target);
                                if (target.id < targets_.size()) {
                                    targets_[target.id] = target;
                                } else {
                                    targets_.resize(target.id + 1);
                                    targets_[target.id] = target;
                                }
                            }
                            break;
                        
                        case dlink::PKT_AMMO: memcpy(&ammoCfg_, payload, sizeof ammoCfg_); hasAmmo_.store(true); break;
                        case dlink::PKT_CONFIG: memcpy(&droneCfg_, payload, sizeof droneCfg_); hasConfig_.store(true); break;
                    }
                }
            }
        }
    }
}

dlink::Telemetry UartLink::getTelemetry() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return telemetry_;
}

dlink::TargetPos UartLink::getTarget(int idx) const {
    std::lock_guard<std::mutex> lk(mtx_);
    return targets_.at(idx);
}

int UartLink::getTargetCount() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return static_cast<int>(targets_.size());
}

dlink::AmmoCfg UartLink::getAmmoCfg() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return ammoCfg_;
}

dlink::DroneCfg UartLink::getDroneCfg() const {
    std::lock_guard<std::mutex> lk(mtx_);
    return droneCfg_;
}

bool UartLink::isThreadReady() const {
    return ready_.load();
}

void UartLink::start() {
    started_.store(true);
}

void UartLink::stop() {
    stopFlag_.store(true);
}

bool UartLink::hasAmmo() const {
    return hasAmmo_.load();
}

bool UartLink::hasConfig() const {
    return hasConfig_.load();
}

bool UartLink::hasTelemetry() const {
    return hasTelemetry_.load();
}

UartLink::~UartLink() {
    if (fd_ >= 0) {
        close(fd_);
    }
}


