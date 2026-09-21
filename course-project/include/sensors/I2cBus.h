#pragma once
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

class I2cError : public std::runtime_error {
public:
    explicit I2cError(const std::string& message) : std::runtime_error(message) {}
};

class I2cBus {
public:
    I2cBus(const std::string& devicePath, uint8_t address);
    ~I2cBus();

    I2cBus(const I2cBus&) = delete;
    I2cBus& operator=(const I2cBus&) = delete;

    uint8_t readRegister(uint8_t reg) const;
    void readRegisters(uint8_t startReg, uint8_t* buffer, std::size_t count) const;
    void writeRegister(uint8_t reg, uint8_t value) const;

    uint8_t address() const { return address_; }

private:
    int fd_;
    uint8_t address_;
};
