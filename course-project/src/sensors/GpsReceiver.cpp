#include "sensors/GpsReceiver.h"

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <stdexcept>

namespace {

speed_t toSpeed(int baud) {
    switch (baud) {
        case 9600: return B9600;
        case 38400: return B38400;
        case 57600: return B57600;
        case 115200: return B115200;
        default: throw std::invalid_argument("unsupported GPS baud rate: " + std::to_string(baud));
    }
}

constexpr std::size_t kMaxLineLength = 200;  // NMEA 82 символа максимум

}  // namespace

GpsReceiver::GpsReceiver(const std::string& port, int baud) : fd_(-1) {
    const speed_t speed = toSpeed(baud);

    fd_ = open(port.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (fd_ < 0) {
        throw std::runtime_error("could not open GPS port " + port + ": " + std::strerror(errno));
    }

    termios tio{};
    tcgetattr(fd_, &tio);
    cfmakeraw(&tio);
    cfsetispeed(&tio, speed);
    cfsetospeed(&tio, speed);
    tio.c_cflag |= (CLOCAL | CREAD);
    tcsetattr(fd_, TCSANOW, &tio);
}

GpsReceiver::~GpsReceiver() {
    if (fd_ >= 0) close(fd_);
}

void GpsReceiver::poll(int64_t nowMs) {
    char buf[256];
    while (true) {
        const ssize_t n = read(fd_, buf, sizeof buf);
        if (n <= 0) return;

        for (ssize_t i = 0; i < n; ++i) {
            const char c = buf[i];
            if (c == '\n') {
                if (parser_.parseLine(lineBuffer_) && parser_.fix().valid) {
                    lastFixMs_ = nowMs;
                    haveFix_ = true;
                }
                lineBuffer_.clear();
            } else if (c != '\r') {
                lineBuffer_ += c;
                if (lineBuffer_.size() > kMaxLineLength) lineBuffer_.clear();
            }
        }
    }
}

GpsFix GpsReceiver::fix(int64_t nowMs) const {
    GpsFix f = parser_.fix();
    if (!haveFix_ || nowMs - lastFixMs_ > gps::kFixTimeoutMs) {
        f.valid = false;
        f.satellites = 0;
    }
    return f;
}
