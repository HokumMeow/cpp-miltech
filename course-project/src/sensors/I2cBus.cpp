#include "sensors/I2cBus.h"

#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <sstream>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {

std::string hex(uint8_t v) {
    std::ostringstream oss;
    oss << "0x" << std::hex << static_cast<int>(v);
    return oss.str();
}

}  // namespace

I2cBus::I2cBus(const std::string& devicePath, uint8_t address) : fd_(-1), address_(address) {
    fd_ = open(devicePath.c_str(), O_RDWR);
    if (fd_ < 0) {
        throw I2cError("could not be opened " + devicePath + ": " + std::strerror(errno));
    }
    if (ioctl(fd_, I2C_SLAVE, address_) < 0) {
        std::string msg = "could not select address " + hex(address_) + " on " + devicePath +
                           ": " + std::strerror(errno);
        close(fd_);
        fd_ = -1;
        throw I2cError(msg);
    }
}

I2cBus::~I2cBus() {
    if (fd_ >= 0) close(fd_);
}

void I2cBus::writeRegister(uint8_t reg, uint8_t value) const {
    uint8_t buf[2] = {reg, value};
    if (write(fd_, buf, sizeof buf) != static_cast<ssize_t>(sizeof buf)) {
        throw I2cError("device " + hex(address_) + " does not respond (no ACK) when writing to register " +
                        hex(reg) + ": " + std::strerror(errno));
    }
}

void I2cBus::readRegisters(uint8_t startReg, uint8_t* buffer, std::size_t count) const {
    // phase 1: write the register address we want to read from
    if (write(fd_, &startReg, 1) != 1) {
        throw I2cError("device " + hex(address_) + " does not respond (no ACK) when accessing register " +
                        hex(startReg) + ": " + std::strerror(errno));
    }
    // phase 2: read the data from the current register pointer
    ssize_t n = read(fd_, buffer, count);
    if (n < 0 || static_cast<std::size_t>(n) != count) {
        throw I2cError("device " + hex(address_) + " does not respond (no ACK) when reading register " +
                        hex(startReg) + ": " + std::strerror(errno));
    }
}

uint8_t I2cBus::readRegister(uint8_t reg) const {
    uint8_t value = 0;
    readRegisters(reg, &value, 1);
    return value;
}
